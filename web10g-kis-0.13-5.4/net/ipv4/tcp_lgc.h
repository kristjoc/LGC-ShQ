#ifndef _TCP_LGC_H_
#define _TCP_LGC_H_

#include <linux/types.h>
#include <net/net_namespace.h>

#define LGC_LUT_SIZE 65537U
extern const u32 log_lut[LGC_LUT_SIZE];
extern const u32 pow_lut[LGC_LUT_SIZE];
extern const u32 exp_lut[LGC_LUT_SIZE];

extern int sysctl_lgc_max_rate[1] __read_mostly;

inline u32 lgc_log_lut_lookup(u32);
inline u32 lgc_pow_lut_lookup(u32);
inline u32 lgc_exp_lut_lookup(u32);

inline int lgc_register_sysctl(void);
inline void lgc_unregister_sysctl(void);

#endif