#include "backend/color.hh"
#include "mach/frame.hh"
#include <climits>
#include <utility>
#include "utils/assert.hh"
#include "utils/random.hh"
#include "utils/algo.hh"

namespace backend
{
namespace regalloc
{
struct allocator {
	allocator(
		std::unordered_map<utils::temp, utils::temp_pair_set> move_list,
		utils::temp_pair_set worklist_moves)
	    : move_list_(move_list), moves_wkl_(worklist_moves)
	{
	}

	void add_edge(utils::temp u, utils::temp v)
	{
		if (u == v || adjacency_set_.count({u, v}))
			return;

		adjacency_set_.insert({u, v});
		adjacency_set_.insert({v, u});

		auto precolored = mach::temp_map();
		if (precolored.find(u) == precolored.end()) {
			adjacency_list_[u] += v;
			degree_[u] += 1;
		}
		if (precolored.find(v) == precolored.end()) {
			adjacency_list_[v] += u;
			degree_[v] += 1;
		}
	}

	utils::temp_pair_set node_moves(utils::temp t)
	{
		return move_list_[t].intersect(active_moves_ + moves_wkl_);
	}

	bool move_related(utils::temp t) { return node_moves(t).size() != 0; }

	// initial must be empty after this according to Appel
	void make_worklist(utils::uset<utils::temp> &initial)
	{
		for (auto n : initial) {
			unsigned degree = degree_[n];

			if (degree >= mach::reg_count())
				spill_wkl_ += n;
			else if (move_related(n))
				freeze_wkl_ += n;
			else
				simplify_wkl_ += n;
		}
	}

	void build(std::vector<ifence_graph::node_type> &nodes)
	{
		for (auto &node : nodes) {
			auto temp = node->value_;

			for (auto pred : node.pred_)
				add_edge(temp, nodes[pred]->value_);
			for (auto succ : node.succ_)
				add_edge(temp, nodes[succ]->value_);
		}

		for (auto [t, _] : mach::temp_map())
			degree_[t] = UINT_MAX;
	}

	utils::temp_set adjacent(utils::temp t)
	{
		return adjacency_list_[t]
		       - (coalesced_nodes_
			  + std::pair(select_stack_.begin(),
				      select_stack_.end()));
	}

	void enable_moves(const utils::temp_set &nodes)
	{
		for (auto &n : nodes) {
			for (auto &m : node_moves(n)) {
				if (!active_moves_.count(m))
					continue;
				active_moves_ -= m;
				moves_wkl_ += m;
			}
		}
	}

	void decrement_degree(utils::temp m)
	{
		ASSERT(degree_[m] > 0, "Can't decrement 0");
		auto d = degree_[m]--;
		if (d == mach::reg_count()) {
			enable_moves(adjacent(m) + m);
			spill_wkl_ -= m;
			if (move_related(m))
				freeze_wkl_ += m;
			else
				simplify_wkl_ += m;
		}
	}

	void simplify()
	{
		ASSERT(simplify_wkl_.size() > 0, "Simplify worklist empty");
		// XXX: Heuristic to choose n?
		auto n = *utils::choose(simplify_wkl_.begin(),
					simplify_wkl_.end());
		simplify_wkl_ -= n;
		select_stack_.push_back(n);

		for (auto m : adjacent(n)) {
			decrement_degree(m);
		}
	}

	utils::temp get_alias(utils::temp n)
	{
		if (coalesced_nodes_.count(n))
			return get_alias(alias_[n]);
		return n;
	}

	void freeze_moves(utils::temp u)
	{
		for (auto m : node_moves(u)) {
			auto [x, y] = m;
			auto v = get_alias(y) == get_alias(u) ? get_alias(x)
							      : get_alias(y);
			active_moves_ -= m;
			frozen_moves_ += m;

			if (node_moves(v).size() == 0
			    && degree_[v] < mach::reg_count()) {
				freeze_wkl_ -= v;
				simplify_wkl_ += v;
			}
		}
	}

	void select_spill()
	{
		// XXX: Add heuristic
		auto beg = spill_wkl_.begin();
		std::advance(beg,
			     utils::rand<size_t>(0, spill_wkl_.size() - 1));
		auto m = *beg;

		spill_wkl_ -= m;
		simplify_wkl_ += m;
		freeze_moves(m);
	}

	void freeze()
	{
		// XXX: Add heuristic
		auto u = *utils::choose(freeze_wkl_.begin(), freeze_wkl_.end());
		freeze_wkl_ -= u;
		simplify_wkl_ += u;
		freeze_moves(u);
	}

	void add_work_list(utils::temp u)
	{
		if (mach::temp_map().count(u) == 0 && !move_related(u)
		    && degree_[u] < mach::reg_count()) {
			freeze_wkl_ -= u;
			simplify_wkl_ += u;
		}
	}

	bool ok(utils::temp t, utils::temp r)
	{
		return degree_[t] < mach::reg_count()
		       || mach::temp_map().count(t)
		       || adjacency_set_.count(std::pair(t, r));
	}

	bool conservative(utils::temp_set nodes)
	{
		unsigned k = 0;
		for (auto &n : nodes) {
			if (degree_[n] >= mach::reg_count())
				k++;
		}
		return k < mach::reg_count();
	}

	void combine(utils::temp u, utils::temp v)
	{
		if (freeze_wkl_.count(v))
			freeze_wkl_ -= v;
		else {
			ASSERT(spill_wkl_.count(v),
			       "Removing but it isn't here");
			spill_wkl_ -= v;
		}
		coalesced_nodes_ += v;
		alias_[v] = u;
		move_list_[u] += move_list_[v];

		for (auto &t : adjacent(v)) {
			add_edge(t, u);
			decrement_degree(t);
		}

		if (degree_[u] >= mach::reg_count() && freeze_wkl_.count(u)) {
			freeze_wkl_ -= u;
			simplify_wkl_ += u;
			freeze_moves(u);
		}
	}

	void coalesce()
	{
		auto tmp_moves_wkl_ = moves_wkl_;
		for (auto m : tmp_moves_wkl_) {
			auto [x, y] = m;
			x = get_alias(x);
			y = get_alias(y);

			auto precolored = mach::temp_map();
			auto [u, v] = precolored.count(y) ? std::pair(y, x)
							  : std::pair(x, y);

			moves_wkl_ -= m;
			if (u == v) {
				coalesced_moves_ += m;
				add_work_list(u);
			} else if (precolored.count(v)
				   || adjacency_set_.count(std::pair(u, v))) {
				constrained_moves_ += m;
				add_work_list(u);
				add_work_list(v);
			} else if ((precolored.count(u)
				    // u is a reference name, and can't be
				    // captured by a lambda.
				    && utils::all_of(adjacent(v),
						     [this, u = u](auto t) {
							     return ok(t, u);
						     }))
				   || (!precolored.count(u)
				       && conservative(adjacent(u)
						       + adjacent(v)))) {
				coalesced_moves_ += m;
				combine(u, v);
				add_work_list(u);
			} else
				active_moves_ += m;
		}
	}

	utils::temp_endomap assign_colors()
	{
		utils::temp_endomap colors;
		auto precolored = mach::temp_map();
		utils::temp_set precolored_set;

		for (auto [t, _] : precolored) {
			colors.emplace(t, t);
			precolored_set += t;
		}

		while (select_stack_.size()) {
			auto n = select_stack_.back();
			select_stack_.pop_back();

			utils::temp_set ok_colors(mach::registers());

			for (auto &w : adjacency_list_[n]) {
				auto col = colored_nodes_ + precolored_set;
				if (col.count(get_alias(w)))
					ok_colors -= colors[get_alias(w)];
			}

			if (ok_colors.size() == 0)
				spill_nodes_.push_back(n);
			else {
				colored_nodes_ += n;
				// XXX: Heuristic?
				colors[n] = *utils::choose(ok_colors.begin(),
							   ok_colors.end());
			}
		}

		for (auto &n : coalesced_nodes_)
			colors[n] = colors[get_alias(n)];

		return colors;
	}

	utils::temp_endomap
	allocate(utils::temp_set initial,
		 std::vector<ifence_graph::node_type> &nodes)
	{
		build(nodes);
		make_worklist(initial);

		while (true) {
			if (simplify_wkl_.size())
				simplify();
			else if (moves_wkl_.size())
				coalesce();
			else if (freeze_wkl_.size())
				freeze();
			else if (spill_wkl_.size())
				select_spill();
			else
				break;
		}

		return assign_colors();
	}

	std::unordered_map<utils::temp, utils::temp_pair_set> move_list_;
	utils::temp_pair_set moves_wkl_;
	utils::temp_pair_set constrained_moves_;
	std::vector<utils::temp> spill_nodes_;
	utils::temp_set colored_nodes_;
	utils::temp_set coalesced_nodes_;
	utils::temp_pair_set coalesced_moves_;
	utils::temp_pair_set active_moves_;
	utils::temp_pair_set frozen_moves_;

	utils::temp_endomap alias_;

	std::vector<utils::temp> select_stack_;

	utils::temp_set spill_wkl_;
	utils::temp_set freeze_wkl_;
	utils::temp_set simplify_wkl_;

	std::unordered_map<utils::temp, unsigned> degree_;
	utils::temp_pair_set adjacency_set_;
	std::unordered_map<utils::temp, utils::temp_set> adjacency_list_;
};

coloring_out color(backend::ifence_graph &ifence, utils::temp_set initial)
{
	(void)ifence;
	(void)initial;

	auto nodes = ifence.graph_.nodes_;
	allocator allo(ifence.move_list_, ifence.worklist_moves_);

	auto allocation = allo.allocate(initial, nodes);
	return {
		allocation,
		allo.spill_nodes_,
		allo.colored_nodes_,
		allo.coalesced_nodes_,
	};
}
} // namespace regalloc
} // namespace backend