/*
 * Copyright (c) 2004, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */
#ifndef CONTIKI_H_
#define CONTIKI_H_

#include "contiki-version.h"
#include "contiki-conf.h"
#include "contiki-default-conf.h"

#include "sys/process.h"
#include "sys/autostart.h"

#include "sys/timer.h"
#include "sys/ctimer.h"
#include "sys/etimer.h"
#include "sys/rtimer.h"

#include "sys/pt.h"

#include "sys/procinit.h"

#include "sys/loader.h"
#include "sys/clock.h"

#include "sys/energest.h"


/// Configuration for tests
/// Author: Fabricio N. de Godoi
#ifndef CONTIKI_CONFIGURATION_TEST
#define CONTIKI_CONFIGURATION_TEST

#ifndef ENABLED
#define ENABLED 1
#endif
#ifndef DISABLED
#define DISABLED 0
#endif

//*** Configuration for development tests, options
#define CONTIKI_FWD_CHK 	ENABLED
#define CONTIKI_FINBUFF		DISABLED

/// String with name of test running
#if (CONTIKI_FWD_CHK!=ENABLED && CONTIKI_FINBUFF!=ENABLED)
#define CONTIKI_TYPE "DEFAULT"
#elif (CONTIKI_FWD_CHK==ENABLED && CONTIKI_FINBUFF!=ENABLED)
#define CONTIKI_TYPE "FORWARD"
#elif (CONTIKI_FWD_CHK!=ENABLED && CONTIKI_FINBUFF==ENABLED)
#define CONTIKI_TYPE "BUFFER"
#elif (CONTIKI_FWD_CHK==ENABLED && CONTIKI_FINBUFF==ENABLED)
#define CONTIKI_TYPE "FWD_BUFF"
#else
#define CONTIKI_TYPE "NONE"
#endif

#endif //!CONTIKI_CONFIGURATION_TEST


#endif /* CONTIKI_H_ */
