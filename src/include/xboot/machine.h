#ifndef __MACHINE_H__
#define __MACHINE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <xboot.h>

enum map_type_t {
	MT_DEVICE	= 0,
	MT_MEMORY	= 1,
};

struct mmap_t {
	const char * name;
	virtual_addr_t virt;
	physical_addr_t phys;
	physical_size_t size;
	enum map_type_t type;
};

struct machine_t {
	struct kobj_t * kobj;
	const char * name;
	const char * desc;
	const struct mmap_t * map;
	bool_t (*detect)(void);
	bool_t (*poweron)(void);
	bool_t (*poweroff)(void);
	bool_t (*reboot)(void);
	bool_t (*sleep)(void);
	bool_t (*cleanup)(void);
	const char * (*uniqueid)(void);
	int (*keygen)(const char * msg, void * key);
};

bool_t register_machine(struct machine_t * mach);
bool_t unregister_machine(struct machine_t * mach);

struct machine_t * get_machine(void);
bool_t machine_poweroff(void);
bool_t machine_reboot(void);
bool_t machine_sleep(void);
bool_t machine_cleanup(void);
const char * machine_uniqueid(void);
int machine_keygen(const char * msg, void * key);

void subsys_init_machine(void);

#ifdef __cplusplus
}
#endif

#endif /* __MACHINE_H__ */
