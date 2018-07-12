#ifndef _APP_GENSOFT_DIALOG_SERVER_LIBSRC_ATOMIC_OP_H_
#define _APP_GENSOFT_DIALOG_SERVER_LIBSRC_ATOMIC_OP_H_

#include <stdint.h>
/** 
 * \brief InterlockedAdd
 *   32位数的原子加
 * 
 * \param i     [in] 增量
 * \param v     [in] 待操作的数
 * 
 * \return  返回两数之和 @v + i
 */
static __inline__ int32_t InterlockedAdd(int32_t i, int32_t *v)
{
    #define LOCK_PREFIX "lock ; "
	int32_t __i = i;
	__asm__ __volatile__(
		LOCK_PREFIX "xaddl %0, %1"
		:"+r" (i), "+m" (*v)
		: : "memory");
	return i + __i;
}

/** 
 * \brief InterlockedSub
 *   32位数的原子减
 * 
 * \param i     [in] 减量
 * \param v     [in] 待操作的数
 * 
 * \return  返回两数之差 @v - i
 */
static __inline__ int32_t InterlockedSub(int32_t i, int32_t *v)
{
	return InterlockedAdd(-i,v);
}

/** 
 * \brief InterlockedAdd
 *   64位数的原子加
 * 
 * \param i     [in] 增量
 * \param v     [in] 待操作的数
 * 
 * \return  返回两数之和 @v + i
 */
static __inline__ int64_t InterlockedAdd64(int64_t i, int64_t *v)
{
	#define LOCK_PREFIX "lock ; "
	int64_t __i = i;
	__asm__ __volatile__(
		LOCK_PREFIX "xaddq %0, %1;"
		:"=r"(i)
		:"m"(*v), "0"(i));
	return i + __i;
}

/** 
 * \brief InterlockedSub
 *   64位数的原子减
 * 
 * \param i     [in] 减量
 * \param v     [in] 待操作的数
 * 
 * \return  返回两数之差 @v - i
 */
static __inline__ int64_t InterlockedSub64(int64_t i, int64_t *v)
{
	return InterlockedAdd64(-i,v);
}

/*
The InterlockedCompareExchange function performs an atomic comparison of the Destination
value with the Comperand value. If the Destination value is equal to the Comperand value, 
the Exchange value is stored in the address specified by Destination. Otherwise, no operation is performed.
The function returns the initial value of the destination.
*/
#define __xg(x) ((volatile uint64_t *)(x))

static __inline__ uint64_t InterlockedCompareExchange64(volatile uint64_t *Destination, uint64_t Exchange, uint64_t Comperand)
{
	uint64_t prev;
	__asm__ __volatile__(LOCK_PREFIX "cmpxchgq %1,%2"
		: "=a"(prev)
		: "r"(Exchange), "m"(*__xg(Destination)), "0"(Comperand)
		: "memory");
	return prev;
}

/// 自增和自减
#define InterlockedIncrement64(v)  (InterlockedAdd64(1ll,v))
#define InterlockedDecrement64(v)  (InterlockedSub64(1ll,v))
#define InterlockedIncrement(v)  (InterlockedAdd(1,v))
#define InterlockedDecrement(v)  (InterlockedSub(1,v))

#define xchg(ptr,v) ((__typeof__(*(ptr)))__xchg((unsigned long)(v),(ptr),sizeof(*(ptr))))

struct __xchg_dummy { unsigned long a[100]; };
#define __pure_xg(x) ((struct __xchg_dummy *)(x))

/*
 * Note: no "lock" prefix even on SMP: xchg always implies lock anyway
 * Note 2: xchg has side effect, so that attribute volatile is necessary,
 *    but generally the primitive is invalid, *ptr is output argument. --ANK
 */
static __inline__ unsigned long __xchg(unsigned long x, volatile void * ptr, int size)
{
    switch (size) {
        case 1:
            __asm__ __volatile__("xchgb %b0,%1"
                :"=q" (x)
                :"m" (*__pure_xg(ptr)), "0" (x)
                :"memory");
            break;
        case 2:
            __asm__ __volatile__("xchgw %w0,%1"
                :"=r" (x)
                :"m" (*__pure_xg(ptr)), "0" (x)
                :"memory");
            break;
        case 4:
            __asm__ __volatile__("xchgl %k0,%1"
                :"=r" (x)
                :"m" (*__pure_xg(ptr)), "0" (x)
                :"memory");
            break;  
            
#ifdef __x86_64__
        case 8:
            __asm__ __volatile__("xchgq %0,%1"
                :"=r" (x)
                :"m" (*__pure_xg(ptr)), "0" (x)
                :"memory");
            break;
#endif
    }

    return x;
}

static __inline__ uint8_t InterlockedExchange8 (volatile uint8_t * target, uint8_t value) 
{
    return xchg(target, value) ;
}

static __inline__ uint16_t InterlockedExchange16 (volatile uint16_t * target, uint16_t value) 
{
    return xchg(target, value) ;
}

static __inline__ uint32_t InterlockedExchange32 (volatile uint32_t * target, uint32_t value)
{
    return xchg(target, value) ;
}

static __inline__ uint64_t InterlockedExchange64 (volatile uint64_t * target, uint64_t value)
{
    return xchg(target, value) ;
}


#define HXAtomicIncRetINT32(pNum) InterlockedIncrement(pNum) 
#define HXAtomicDecRetINT32(pNum) InterlockedDecrement(pNum)  
#endif
