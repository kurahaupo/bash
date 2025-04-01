// Controls for GNU Indent

// Preferred style for this project;
// mostly the GNU default style with a few minor deviations.

-brs
-cp 17	// avoid line break after indented #else & #endif
-cs	// space after cast OR: -ncs	// no space after cast
-l 128
-nbc
-ntac
-ppi 2
-sar	// comment this out if using indent before version 2.2.12

// Project typenames

-T ARITH_COM
-T ARITH_FOR_COM
-T ARRAY
-T ARRAY_ELEMENT
-T BASH_INPUT
-T BUCKET_CONTENTS
-T BUFFERED_STREAM
-T BUILTIN_DESC
-T CASE_COM
-T COLOR_EXT_TYPE
-T COMMAND
-T COMPSPEC
-T COND_COM
-T CONNECTION
-T COPROC_COM
-T Coproc
-T DEF_FILE
-T ELEMENT
-T EXPANSION_SAVER
-T EXPFUNC
-T EXPR_CONTEXT
-T FILEINFO
-T FLIST
-T FOR_COM
-T FUNCTION_DEF
-T FUNMAP
-T GENERIC_LIST
-T GROUP_COM
-T HANDLER_ENTRY
-T HASH_TABLE
-T HISTORY_STATE
-T HIST_ENTRY
-T IF_COM
-T INPUT_STREAM
-T INTDEF
-T ITEMLIST
-T JOB
-T JOB_STATE
-T KEYMAP_ENTRY
-T KEYMAP_ENTRY_ARRAY
-T PATH_DATA
-T PATTERN_LIST
-T PFUNC
-T PROCESS
-T PTR_T			// archaic work-around for lack of «void*»
-T PWSTR
-T QSFUNC
-T REDIRECT
-T REDIRECTEE
-T REPL
-T RESOURCE_LIMITS
-T SAVED_VAR
-T SELECT_COM
-T SHELL_VAR
-T SIMPLE_COM
-T STRDEF
-T STREAM_SAVER
-T STRINGLIST
-T STRING_INT_ALIST
-T STRING_SAVER
-T STRUCT
-T SUBSHELL_COM
-T SVFUNC
-T TABLEITEM
-T TABLEROW
-T ULCMD
-T UNDO_LIST
-T UNWIND_ELT
-T VARLIST
-T VAR_CONTEXT
-T VISIT
-T WAIT
-T WHILE_COM
-T WORD_DESC
-T WORD_LIST
-T __COLLSYM			// in lib/glob/collsyms.h, smatch.c
-T _bashfunc
-T accessor_t
-T alias_t
-T arg_type			// enum
-T array_eltstate_t		// in arrayfunc.h
-T arrayind_t
-T breadfunc_t
-T cpelement_t			// only in execute_cmd.c
-T cplist_t			// only in execute_cmd.c
-T creadfunc_t
-T display_style_t
-T foo_t
-T histdata_t
-T mguard_t
-T mk_handler_func_t
-T mp_limb_t
-T mp_twolimb_t
-T node				// typedef struct node_t {} *node;
-T op_result_t
-T opt_def_t
-T opt_get_func_t
-T opt_set_func_t
-T opt_test_func_t
-T option_value_t
-T ps_index_t
-T setopt_get_func_t
-T setopt_set_func_t
-T sh_ae_map_func_t
-T sh_alias_map_func_t
-T sh_assign_func_t
-T sh_builtin_func_t
-T sh_cget_func_t
-T sh_csprint_func_t
-T sh_cunget_func_t
-T sh_free_func_t
-T sh_gcp_func_t
-T sh_getopt_istate_t
-T sh_getopt_state_t
-T sh_glist_func_t
-T sh_icpfunc_t
-T sh_icppfunc_t
-T sh_ignore_func_t
-T sh_input_line_state_t
-T sh_intfunc_t
-T sh_iptrfunc_t
-T sh_iv_item_func_t
-T sh_ivoidfunc_t
-T sh_job_map_func_t
-T sh_load_func_t
-T sh_msg_func_t
-T sh_obj_cache_t
-T sh_parser_state_t
-T sh_resetsig_func_t
-T sh_string_func_t
-T sh_strlist_map_func_t
-T sh_sv_func_t
-T sh_timer
-T sh_unload_func_t
-T sh_uwfunc_t
-T sh_var_assign_func_t
-T sh_var_map_func_t
-T sh_var_value_func_t
-T sh_vcpfunc_t
-T sh_vcppfunc_t
-T sh_vintfunc_t
-T sh_vmsg_func_t
-T sh_voidfunc_t
-T sh_vptrfunc_t
-T sh_wassign_func_t
-T sh_wdesc_func_t
-T sh_wlist_func_t
-T shopt_set_func_t
-T show_func_t
-T tilde_hook_func_t
-T transmem_block_t
-T wchar_t_directive
-T wchar_t_directives

-T char_directive
-T char_directives
-T chartype
-T sighandler

// LibMalloc library

-T genptr_t			// only in lib/malloc/OLD/gmalloc.c
-T ma_table_t			// only in lib/malloc/table.[ch]
-T mr_table_t			// only in lib/malloc/table.[ch]
-T header			// only in lib/malloc/alloca.c
-T malloc_info			// only in lib/malloc/OLD/ogmalloc.c & gmalloc.c

// LibIntl library

-T __action_fn_t		// only in lib/intl/tsearch.c
-T __compar_fn_t		// only in lib/intl/tsearch.c
-T __free_fn_t			// only in lib/intl/tsearch.c
-T const_node			// only in lib/intl/tsearch.c
-T mpn_t			// only in lib/intl/vasnprintf.c
-T node_t			// only in lib/intl/tsearch.c
-T once_flag			// only in lib/intl/lock.h & setlocal-lock.c
-T range_t			// only in lib/intl/setlocale.c

-T gl_cv_header_locale_has_locale_t
-T gl_lock_t
-T gl_once_t
-T gl_recursive_lock_t
-T gl_rwlock_t

// Readline library

-T CPFunction
-T CPPFunction
-T Function
-T Keymap
-T SigHandler
-T VFunction
-T _RL_TTY_CHARS
-T _hist_search_func_t
-T _rl_arg_cxt
-T _rl_bool_t
-T _rl_callback_func_t
-T _rl_callback_generic_arg
-T _rl_keyseq_cxt
-T _rl_parser_func_t
-T _rl_readstr_cxt
-T _rl_search_cxt
-T _rl_sigcleanup_func_t
-T _rl_sv_func_t
-T _rl_vimotion_cxt
-T assoc_list			// only in lib/readline/bind.c
-T cc_t				// only in lib/readline/examples/excallback.c
-T complete_sigcleanarg_t	// only in lib/readline/complete.c
-T rl_command_func_t
-T rl_compdisp_func_t
-T rl_compdisplay_func_t
-T rl_compentry_func_t
-T rl_compignore_func_t
-T rl_completion_func_t
-T rl_cpcpfunc_t
-T rl_cpcppfunc_t
-T rl_cpifunc_t
-T rl_cpvfunc_t
-T rl_delete_t
-T rl_deprep_t
-T rl_dequote_func_t
-T rl_getc_func_t
-T rl_hook_func_t
-T rl_icpfunc_t
-T rl_icppfunc_t
-T rl_intfunc_t
-T rl_ivoidfunc_t
-T rl_linebuf_func_t
-T rl_macro_print_func_t
-T rl_modterm_func_t
-T rl_quote_func_t
-T rl_reset_t
-T rl_vcpfunc_t
-T rl_vcppfunc_t
-T rl_vintfunc_t
-T rl_voidfunc_t
-T sighandler_cxt		// only in lib/readline/signals.c

-T sort_element			// only in examples/loadables/asort.c
-T unix_link_syscall_t		// only in examples/loadables/ln.c

// Flex/Lex & Yacc/Bison typenames

-T YYSTYPE
-T yy_state_fast_t
-T yy_state_t
-T yysigned_char
-T yysymbol_kind_t
-T yytoken_kind_t

// from external GLW thread library

-T glwthread_initguard_t
-T glwthread_mutex_t
-T glwthread_once_t
-T glwthread_recmutex_t
-T glwthread_rwlock_t

// MSVC typenames (only used in the platform files)

-T DWORD
-T GetUserDefaultUILanguage_func
-T GetUserPreferredUILanguages_func
-T LANGID
-T PULONG
-T ULONG
-T WINAPI

// POSIX typenames

-T DIR				// for opendir etc
-T clock_t			// for times in <sys/times.h>
-T cnd_t			// for cnd_init, cnd_signal, cnd_broadcast, cnd_wait, cnd_timedwait, cnd_destroy in <thread.h>
-T dev_t			// for fstat, fstatat, lstat, stat in <sys/stat.h>+<unistd.h>
-T gid_t
-T iconv_t
-T ino_t
-T mbstate_t			// for mbsrtowcs in <wchar.h>
-T mode_t			// from <fcntl.h>
-T mtx_t			// from <threads.h>
-T nls_uint32
-T off_t			// lseek
-T pid_t
-T pthread_cond_t
-T pthread_key_t
-T pthread_mutex_t
-T pthread_mutexattr_t
-T pthread_once_t
-T pthread_rwlock_t
-T pthread_rwlockattr_t
-T pthread_t
-T regex_t
-T regmatch_t
-T sigset_t
-T socklen_t
-T speed_t			// tcgetattr
-T ssize_t			// size_t or (-1), for syscalls that return length-or-error
-T tcflag_t			// tcgetattr
-T time_t
-T uid_t

// Fake POSIX work-alike typenames

-T __gconv_t			// from <gconv.h>
-T caddr_t			// from <linux/coda.h>
-T in_addr_t			// from <netinet/in.h>
-T shl_t			// only in CWRU/misc/hpux10-dlfcn.h (for HPUX)

// pseudo-generic names

-T __ptr_t
-T dummy
-T int_ptr
-T iptr
-T long_ptr
-T pointer

// ISO C standard typenames

-T FILE				// for fopen etc; both C89 & POSIX-2001
-T _Bool
-T bool
-T fpos_t			// fgetpos & fsetpos (improved versions of ftell & fseek)
-T imaxdiv_t
-T int128_t
-T int16_t
-T int32_t
-T int64_t
-T int8_t
-T int_least16_t
-T int_least8_t
-T intmax_t
-T intptr_t
-T locale_t			// in <stdlib.h>, <locale.h>, <wchar.h>, <wctype.h>
-T ptrdiff_t
-T sig_atomic_t
-T size_t
-T uint128_t
-T uint16_t
-T uint32_t
-T uint64_t
-T uint8_t
-T uint_least16_t
-T uint_least8_t
-T uintmax_t
-T uintptr_t
-T wchar_t
-T wctype_t
-T wint_t

// Fake ISO C work-alike typenames

-T __INT_LEAST16_TYPE__
-T __INT_LEAST8_TYPE__
-T __UINT_LEAST16_TYPE__
-T __UINT_LEAST8_TYPE__
-T __malloc_ptrdiff_t
-T __malloc_size_t
-T bits16_t
-T bits32_t
-T bits64_t
-T char32_t
-T floatmax_t
-T procenv_t			// #define for either jmp_buf or sigjmp_buf
-T u_bits16_t
-T u_bits32_t
-T u_int32_t
-T yytype_int16
-T yytype_int8
-T yytype_uint16
-T yytype_uint8

// Things that are NOT actually typedefs; left here as a reminder not to add them!

// _LC_core_data_locale_t	// doc only
// index_t			// doc only
// mbchar_t			// doc only
// shell_input_line_state_t	// doc only
// sigcleanarg_t		// doc only
// sv_func_tk			// doc only
// wassign_func_t		// doc only

// GNULIB_defined_mbstate_t	// not a typedef; actually a macro - only used for #ifdef
// _LC_locale_t			// not a typedef; actually a struct tag
// __locale_t			// not a typedef; actually a struct tag
// _sh_input_line_state_t	// not a typedef; actually a struct tag
// _sh_parser_state_t		// not a typedef; actually a struct tag
// globsort_t			// not a typedef; actually a struct tag
// hash_wfunc			// not a typedef; actually a function pointer
// known_translation_t		// not a typedef; actually a struct tag
// print_clock_t		// not a typedef; actually a function name
// xcast_size_t			// not a typedef; actually a function-like macro for min(X, MAX_SIZE_T)

// free_t			// not a typedef; actually a variable
// lhs_t			// not a typedef; actually a variable
// rhs_t			// not a typedef; actually a variable

// __libc_lock_recursive_t	// not a typedef; actually an #ifdef; #define to enable gl_recursive_lock_t
// __libc_lock_t		// not a typedef; actually an #ifdef; #define to enable gl_lock_t

// unconfined_t			// not C; actually an SElinux context

// ac_cv_have_sig_atomic_t	// not C; only in configure
// ac_cv_sizeof_intmax_t	// not C; only in configure
// ac_cv_sizeof_size_t		// not C; only in configure
// ac_cv_sizeof_wchar_t		// not C; only in configure
// ac_cv_type_bits16_t		// not C; only in configure
// ac_cv_type_bits32_t		// not C; only in configure
// ac_cv_type_bits64_t		// not C; only in configure
// ac_cv_type_gid_t		// not C; only in configure
// ac_cv_type_intmax_t		// not C; only in configure
// ac_cv_type_intptr_t		// not C; only in configure
// ac_cv_type_mode_t		// not C; only in configure
// ac_cv_type_off_t		// not C; only in configure
// ac_cv_type_pid_t		// not C; only in configure
// ac_cv_type_pthread_rwlock_t	// not C; only in configure
// ac_cv_type_ptrdiff_t		// not C; only in configure
// ac_cv_type_quad_t		// not C; only in configure
// ac_cv_type_size_t		// not C; only in configure
// ac_cv_type_ssize_t		// not C; only in configure
// ac_cv_type_time_t		// not C; only in configure
// ac_cv_type_u_bits16_t	// not C; only in configure
// ac_cv_type_u_bits32_t	// not C; only in configure
// ac_cv_type_uid_t		// not C; only in configure
// ac_cv_type_uintmax_t		// not C; only in configure
// ac_cv_type_uintptr_t		// not C; only in configure
// ac_cv_type_wchar_t		// not C; only in configure
// ac_t				// not C; only in configure
// bash_cv_sizeof_quad_t	// not C; only in configure
// bash_cv_type_clock_t		// not C; only in configure
// bash_cv_type_int32_t		// not C; only in configure
// bash_cv_type_intmax_t	// not C; only in configure
// bash_cv_type_quad_t		// not C; only in configure
// bash_cv_type_sig_atomic_t	// not C; only in configure
// bash_cv_type_sigset_t	// not C; only in configure
// bash_cv_type_socklen_t	// not C; only in configure
// bash_cv_type_u_int32_t	// not C; only in configure
// bash_cv_type_uintmax_t	// not C; only in configure
// bash_cv_type_wchar_t		// not C; only in configure
// bash_cv_type_wctype_t	// not C; only in configure
// bash_cv_type_wint_t		// not C; only in configure
// foo_t			// not C; only in configure
// gt_cv_c_intmax_t		// not C; only in configure
// gt_cv_c_wchar_t		// not C; only in configure
// gt_cv_c_wint_t		// not C; only in configure
// quad_t			// not C; only in configure
// rlim_t			// not C; only in configure

// Typename that's only used in configure.
// In the C code, it's a var name.
// sigfunc			// only in configure and docs
