volatile int g_debugSocket;
#define BUF_SIZE 1024
char kprintfbuffer[BUF_SIZE];
char bprintfbuffer[BUF_SIZE];
volatile int bprintflength;
volatile int (*sys_sendto)(void *td,void *uap);

#define printf(format, ...) \
	do { \
		char buffer[BUF_SIZE]; \
		int size = sprintf(buffer, format, ##__VA_ARGS__); \
		sceNetSend(g_debugSocket, buffer, size, 0); \
	} while(0)

#define kprintf(format, ...) \
	do { \
		if (sys_sendto != 0 && g_debugSocket != 0) { \
			int size = sprintf(kprintfbuffer, format, ##__VA_ARGS__); \
			struct sendto_args args = { g_debugSocket, kprintfbuffer, size, 0, NULL, 0 }; \
			sys_sendto(td, &args); \
		} else { \
			if (snprintf(NULL, 0, format, ##__VA_ARGS__) + bprintflength < BUF_SIZE); \
				bprintflength += sprintf(bprintfbuffer+bprintflength, format, ##__VA_ARGS__); \
		} \
	} while(0)

#define bflush() \
	do { \
		if (sys_sendto != 0 && g_debugSocket !=0 && bprintflength > 0) { \
			struct sendto_args args = { g_debugSocket, bprintfbuffer, bprintflength, 0, NULL, 0 }; \
			sys_sendto(td, &args); \
			bprintflength = 0; \
			memset(bprintfbuffer, 0, sizeof(bprintfbuffer)); \
		} \
	} while(0)

struct sendto_args {
	int	s;
	void *	buf;
	size_t	len;
	int	flags;
	void *	to;
	int	tolen;
};


struct auditinfo_addr {
	/*
	8	ai_auid;
	16	ai_mask;
	28	ai_termid;
	8	ai_asid;
	8	ai_flags;
	*/
	char useless[68];
};

struct filedesc {
	void 	*fd_ofiles;	/* file structures for open files */
	void	*fd_ofileflags;		/* per-process open file flags */
	uint64_t *fd_cdir;		/* current directory */
	uint64_t *fd_rdir;		/* root directory */
	uint64_t *fd_jdir;		/* jail root directory */
};


struct ucred {
	int	cr_ref;			// reference count  		0
	int	cr_uid;			// effective user id 		4
	int	cr_ruid;		// real user id 		8
	int	cr_svuid;		// saved user id 		12
	int	cr_ngroups;		// number of groups 		16
	int	cr_rgid;		// real group id 		20
	char 	unk1[24];
	uint64_t	*cr_prison;	// jail(2) 			48
	char 	unk2[224];
	int	*cr_groups;		// groups 			280
	int	cr_agroups;		// Available groups 		288
};

struct proc {
	char unk1[64];
	struct ucred *p_ucred;
	struct filedesc	*p_fd;	//untested
};

struct thread {
	void *unk1;
	struct proc *td_proc;
	char unk2[288];
	struct ucred *td_ucred;
};

struct knote;

struct kevent {
	char useless[32];
};
struct filterops {
	int	f_isfd;		/* true if ident == filedescriptor */
	int	(*f_attach)(struct knote *kn);
	void	(*f_detach)(struct knote *kn);
	int	(*f_event)(struct knote *kn, long hint);
	void	(*f_touch)(struct knote *kn, struct kevent *kev, unsigned long type);
};

struct knote {
	int64_t 		*kn_link;	/* for kq */
	int64_t 		*kn_selnext;	/* for struct selinfo */
	uint64_t 		*kn_knlist;	/* f_attach populated */
	uint64_t 		*kn_tqe;
	uint64_t 		*kn_tqe2;
	uint64_t		*kn_kq;		/* which queue we are on */
	struct 			kevent kn_kevent;
	int			kn_status;	/* protected by kq lock */
	int			kn_sfflags;	/* saved filter flags */
	intptr_t		kn_sdata;	/* saved data field */
	union {
		uint64_t *p_fp;	/* file data pointer */
		uint64_t *p_proc;	/* proc pointer */
		uint64_t *p_aio;	/* AIO job pointer */
		uint64_t *p_lio;	/* LIO job pointer */ 
	} kn_ptr;
	struct			filterops *kn_fop;
	void			*kn_hook;
	int			kn_hookid;
};
