/*
 * lives.h
 *
 * Created: 2019/5/19 星期日 下午 3:09:26
 *  Author: Administrator
 */ 


#ifndef LIVES_H_
#define LIVES_H_

#include <stdint.h>

void init_lives(void);
void reduce_to_lives(uint16_t value);
uint32_t get_lives(void);



#endif /* LIVES_H_ */