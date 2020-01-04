#include "backend/liveness.hh"
#include "utils/uset.hh"
#include "utils/assert.hh"
#include <algorithm>

namespace backend
{
ifence_node::ifence_node(const utils::temp &t) : value_(t) {}

bool ifence_node::operator==(const ifence_node &rhs) const
{
	return value_ == rhs.value_;
}

std::ostream &operator<<(std::ostream &os, const ifence_node &n)
{
	return os << "[label=\"" << n.value_ << "\"]";
}

ifence_graph::ifence_graph(utils::graph<cfgnode> &cfg)
{
	std::vector<utils::temp_set> in(cfg.size());
	std::vector<utils::temp_set> out(cfg.size());

	std::vector<utils::temp_set> new_in(cfg.size());
	std::vector<utils::temp_set> new_out(cfg.size());

	do {
		for (utils::node_id n = 0; n < cfg.size(); n++) {
			new_in[n] = in[n];
			new_out[n] = out[n];

			auto *node = cfg.get(n);
			in[n] = node->use + (out[n] - node->def);

			utils::temp_set children_in;
			for (auto s : cfg.nodes_[n].succ_)
				children_in = children_in + in[s];
			out[n] = children_in;
		}
	} while (new_in != in || new_out != out);

	for (utils::node_id n = 0; n < cfg.size(); n++) {
		auto *node = cfg.get(n);
		if (node->is_move) {
			ASSERT(node->def.size() == 1, "Multiple defs in move");
			ASSERT(node->use.size() <= 1, "Multiple uses in move");
			auto def = node->def.begin();
			auto use = node->use.begin();
			utils::node_id defn = graph_.get_or_insert(*def);
			for (auto b : out[n]) {
				if (use != node->use.end() && *use == b)
					continue;

				utils::node_id outn = graph_.get_or_insert(b);

				graph_.add_edge(defn, outn);
			}
		} else {
			for (auto def : node->def) {
				utils::node_id defn = graph_.get_or_insert(def);
				for (auto b : out[n]) {
					utils::node_id outn =
						graph_.get_or_insert(b);
					graph_.add_edge(defn, outn);
				}
			}
		}
	}
}
} // namespace backend
