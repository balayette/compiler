#pragma once

#define ARM64_NR_io_setup 0
#define ARM64_NR_io_destroy 1
#define ARM64_NR_io_submit 2
#define ARM64_NR_io_cancel 3
#define ARM64_NR_io_getevents 4
#define ARM64_NR_setxattr 5
#define ARM64_NR_lsetxattr 6
#define ARM64_NR_fsetxattr 7
#define ARM64_NR_getxattr 8
#define ARM64_NR_lgetxattr 9
#define ARM64_NR_fgetxattr 10
#define ARM64_NR_listxattr 11
#define ARM64_NR_llistxattr 12
#define ARM64_NR_flistxattr 13
#define ARM64_NR_removexattr 14
#define ARM64_NR_lremovexattr 15
#define ARM64_NR_fremovexattr 16
#define ARM64_NR_getcwd 17
#define ARM64_NR_lookup_dcookie 18
#define ARM64_NR_eventfd2 19
#define ARM64_NR_epoll_create1 20
#define ARM64_NR_epoll_ctl 21
#define ARM64_NR_epoll_pwait 22
#define ARM64_NR_dup 23
#define ARM64_NR_dup3 24
#define ARM64_NR_fcntl 25
#define ARM64_NR_inotify_init1 26
#define ARM64_NR_inotify_add_watch 27
#define ARM64_NR_inotify_rm_watch 28
#define ARM64_NR_ioctl 29
#define ARM64_NR_ioprio_set 30
#define ARM64_NR_ioprio_get 31
#define ARM64_NR_flock 32
#define ARM64_NR_mknodat 33
#define ARM64_NR_mkdirat 34
#define ARM64_NR_unlinkat 35
#define ARM64_NR_symlinkat 36
#define ARM64_NR_linkat 37
#define ARM64_NR_renameat 38
#define ARM64_NR_umount2 39
#define ARM64_NR_mount 40
#define ARM64_NR_pivot_root 41
#define ARM64_NR_nfsservctl 42
#define ARM64_NR_statfs 43
#define ARM64_NR_fstatfs 44
#define ARM64_NR_truncate 45
#define ARM64_NR_ftruncate 46
#define ARM64_NR_fallocate 47
#define ARM64_NR_faccessat 48
#define ARM64_NR_chdir 49
#define ARM64_NR_fchdir 50
#define ARM64_NR_chroot 51
#define ARM64_NR_fchmod 52
#define ARM64_NR_fchmodat 53
#define ARM64_NR_fchownat 54
#define ARM64_NR_fchown 55
#define ARM64_NR_openat 56
#define ARM64_NR_close 57
#define ARM64_NR_vhangup 58
#define ARM64_NR_pipe2 59
#define ARM64_NR_quotactl 60
#define ARM64_NR_getdents64 61
#define ARM64_NR_lseek 62
#define ARM64_NR_read 63
#define ARM64_NR_write 64
#define ARM64_NR_readv 65
#define ARM64_NR_writev 66
#define ARM64_NR_pread64 67
#define ARM64_NR_pwrite64 68
#define ARM64_NR_preadv 69
#define ARM64_NR_pwritev 70
#define ARM64_NR_sendfile 71
#define ARM64_NR_pselect6 72
#define ARM64_NR_ppoll 73
#define ARM64_NR_signalfd4 74
#define ARM64_NR_vmsplice 75
#define ARM64_NR_splice 76
#define ARM64_NR_tee 77
#define ARM64_NR_readlinkat 78
#define ARM64_NR_fstatat64 79
#define ARM64_NR_fstat 80
#define ARM64_NR_sync 81
#define ARM64_NR_fsync 82
#define ARM64_NR_fdatasync 83
#define ARM64_NR_sync_file_range 84
#define ARM64_NR_timerfd_create 85
#define ARM64_NR_timerfd_settime 86
#define ARM64_NR_timerfd_gettime 87
#define ARM64_NR_utimensat 88
#define ARM64_NR_acct 89
#define ARM64_NR_capget 90
#define ARM64_NR_capset 91
#define ARM64_NR_personality 92
#define ARM64_NR_exit 93
#define ARM64_NR_exit_group 94
#define ARM64_NR_waitid 95
#define ARM64_NR_set_tid_address 96
#define ARM64_NR_unshare 97
#define ARM64_NR_futex 98
#define ARM64_NR_set_robust_list 99
#define ARM64_NR_get_robust_list 100
#define ARM64_NR_nanosleep 101
#define ARM64_NR_getitimer 102
#define ARM64_NR_setitimer 103
#define ARM64_NR_kexec_load 104
#define ARM64_NR_init_module 105
#define ARM64_NR_delete_module 106
#define ARM64_NR_timer_create 107
#define ARM64_NR_timer_gettime 108
#define ARM64_NR_timer_getoverrun 109
#define ARM64_NR_timer_settime 110
#define ARM64_NR_timer_delete 111
#define ARM64_NR_clock_settime 112
#define ARM64_NR_clock_gettime 113
#define ARM64_NR_clock_getres 114
#define ARM64_NR_clock_nanosleep 115
#define ARM64_NR_syslog 116
#define ARM64_NR_ptrace 117
#define ARM64_NR_sched_setparam 118
#define ARM64_NR_sched_setscheduler 119
#define ARM64_NR_sched_getscheduler 120
#define ARM64_NR_sched_getparam 121
#define ARM64_NR_sched_setaffinity 122
#define ARM64_NR_sched_getaffinity 123
#define ARM64_NR_sched_yield 124
#define ARM64_NR_sched_get_priority_max 125
#define ARM64_NR_sched_get_priority_min 126
#define ARM64_NR_sched_rr_get_interval 127
#define ARM64_NR_restart_syscall 128
#define ARM64_NR_kill 129
#define ARM64_NR_tkill 130
#define ARM64_NR_tgkill 131
#define ARM64_NR_sigaltstack 132
#define ARM64_NR_rt_sigsuspend 133
#define ARM64_NR_rt_sigaction 134
#define ARM64_NR_rt_sigprocmask 135
#define ARM64_NR_rt_sigpending 136
#define ARM64_NR_rt_sigtimedwait 137
#define ARM64_NR_rt_sigqueueinfo 138
#define ARM64_NR_rt_sigreturn 139
#define ARM64_NR_setpriority 140
#define ARM64_NR_getpriority 141
#define ARM64_NR_reboot 142
#define ARM64_NR_setregid 143
#define ARM64_NR_setgid 144
#define ARM64_NR_setreuid 145
#define ARM64_NR_setuid 146
#define ARM64_NR_setresuid 147
#define ARM64_NR_getresuid 148
#define ARM64_NR_setresgid 149
#define ARM64_NR_getresgid 150
#define ARM64_NR_setfsuid 151
#define ARM64_NR_setfsgid 152
#define ARM64_NR_times 153
#define ARM64_NR_setpgid 154
#define ARM64_NR_getpgid 155
#define ARM64_NR_getsid 156
#define ARM64_NR_setsid 157
#define ARM64_NR_getgroups 158
#define ARM64_NR_setgroups 159
#define ARM64_NR_uname 160
#define ARM64_NR_sethostname 161
#define ARM64_NR_setdomainname 162
#define ARM64_NR_getrlimit 163
#define ARM64_NR_setrlimit 164
#define ARM64_NR_getrusage 165
#define ARM64_NR_umask 166
#define ARM64_NR_prctl 167
#define ARM64_NR_getcpu 168
#define ARM64_NR_gettimeofday 169
#define ARM64_NR_settimeofday 170
#define ARM64_NR_adjtimex 171
#define ARM64_NR_getpid 172
#define ARM64_NR_getppid 173
#define ARM64_NR_getuid 174
#define ARM64_NR_geteuid 175
#define ARM64_NR_getgid 176
#define ARM64_NR_getegid 177
#define ARM64_NR_gettid 178
#define ARM64_NR_sysinfo 179
#define ARM64_NR_mq_open 180
#define ARM64_NR_mq_unlink 181
#define ARM64_NR_mq_timedsend 182
#define ARM64_NR_mq_timedreceive 183
#define ARM64_NR_mq_notify 184
#define ARM64_NR_mq_getsetattr 185
#define ARM64_NR_msgget 186
#define ARM64_NR_msgctl 187
#define ARM64_NR_msgrcv 188
#define ARM64_NR_msgsnd 189
#define ARM64_NR_semget 190
#define ARM64_NR_semctl 191
#define ARM64_NR_semtimedop 192
#define ARM64_NR_semop 193
#define ARM64_NR_shmget 194
#define ARM64_NR_shmctl 195
#define ARM64_NR_shmat 196
#define ARM64_NR_shmdt 197
#define ARM64_NR_socket 198
#define ARM64_NR_socketpair 199
#define ARM64_NR_bind 200
#define ARM64_NR_listen 201
#define ARM64_NR_accept 202
#define ARM64_NR_connect 203
#define ARM64_NR_getsockname 204
#define ARM64_NR_getpeername 205
#define ARM64_NR_sendto 206
#define ARM64_NR_recvfrom 207
#define ARM64_NR_setsockopt 208
#define ARM64_NR_getsockopt 209
#define ARM64_NR_shutdown 210
#define ARM64_NR_sendmsg 211
#define ARM64_NR_recvmsg 212
#define ARM64_NR_readahead 213
#define ARM64_NR_brk 214
#define ARM64_NR_munmap 215
#define ARM64_NR_mremap 216
#define ARM64_NR_add_key 217
#define ARM64_NR_request_key 218
#define ARM64_NR_keyctl 219
#define ARM64_NR_clone 220
#define ARM64_NR_execve 221
#define ARM64_NR_mmap 222
#define ARM64_NR_fadvise64 223
#define ARM64_NR_swapon 224
#define ARM64_NR_swapoff 225
#define ARM64_NR_mprotect 226
#define ARM64_NR_msync 227
#define ARM64_NR_mlock 228
#define ARM64_NR_munlock 229
#define ARM64_NR_mlockall 230
#define ARM64_NR_munlockall 231
#define ARM64_NR_mincore 232
#define ARM64_NR_madvise 233
#define ARM64_NR_remap_file_pages 234
#define ARM64_NR_mbind 235
#define ARM64_NR_get_mempolicy 236
#define ARM64_NR_set_mempolicy 237
#define ARM64_NR_migrate_pages 238
#define ARM64_NR_move_pages 239
#define ARM64_NR_rt_tgsigqueueinfo 240
#define ARM64_NR_perf_event_open 241
#define ARM64_NR_accept4 242
#define ARM64_NR_recvmmsg 243
#define ARM64_NR_arch_specific_syscall 244
#define ARM64_NR_wait4 260
#define ARM64_NR_prlimit64 261
#define ARM64_NR_fanotify_init 262
#define ARM64_NR_fanotify_mark 263
#define ARM64_NR_name_to_handle_at 264
#define ARM64_NR_open_by_handle_at 265
#define ARM64_NR_clock_adjtime 266
#define ARM64_NR_syncfs 267
#define ARM64_NR_setns 268
#define ARM64_NR_sendmmsg 269
#define ARM64_NR_process_vm_readv 270
#define ARM64_NR_process_vm_writev 271
#define ARM64_NR_kcmp 272
#define ARM64_NR_finit_module 273
#define ARM64_NR_sched_setattr 274
#define ARM64_NR_sched_getattr 275
#define ARM64_NR_renameat2 276
#define ARM64_NR_seccomp 277
#define ARM64_NR_getrandom 278
#define ARM64_NR_memfd_create 279
#define ARM64_NR_bpf 280
#define ARM64_NR_execveat 281
#define ARM64_NR_userfaultfd 282
#define ARM64_NR_membarrier 283
#define ARM64_NR_mlock2 284
#define ARM64_NR_copy_file_range 285

#define ARM64_SYSCALL_COUNT (ARM64_NR_copy_file_range + 1)