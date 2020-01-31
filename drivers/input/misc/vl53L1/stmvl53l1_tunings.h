/**************************************************************************
 * Copyright (c) 2016, STMicroelectronics - All Rights Reserved

 License terms: BSD 3-clause "New" or "Revised" License.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

 3. Neither the name of the copyright holder nor the names of its contributors
 may be used to endorse or promote products derived from this software
 without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ****************************************************************************/

/*
 * THIS IS A GENERATED FILE
 */

#ifndef STMVL53L1_TUNINGS_H
#define STMVL53L1_TUNINGS_H

static const int tunings[][2] = {
};

/*Add dedicated tuning parameters for VL53L3*/
static const int tunings_L3[][2] = {
	{VL53L1_TUNINGPARM_HIST_AMB_THRESH_SIGMA_1, 80},
	{VL53L1_TUNINGPARM_HIST_SIGNAL_TOTAL_EVENTS_LIMIT, 50},
	{VL53L1_TUNINGPARM_HIST_SIGMA_THRESH_MM, 180},
	{VL53L1_TUNINGPARM_CONSISTENCY_HIST_MIN_MAX_TOLERANCE_MM, 0},
	{VL53L1_TUNINGPARM_XTALK_DETECT_MAX_VALID_RATE_KCPS, 400},
	{VL53L1_TUNINGPARM_HIST_XTALK_MARGIN_KCPS, 0},
	{VL53L1_TUNINGPARM_XTALK_EXTRACT_NUM_OF_SAMPLES, 13},
	{VL53L1_TUNINGPARM_XTALK_EXTRACT_MAX_VALID_RATE_KCPS, 640},
	{VL53L1_TUNINGPARM_DYNXTALK_SMUDGE_MARGIN, 0},
	{VL53L1_TUNINGPARM_DYNXTALK_NOISE_MARGIN, 100},
	{VL53L1_TUNINGPARM_DYNXTALK_SINGLE_XTALK_DELTA, 2048},
	{VL53L1_TUNINGPARM_DYNXTALK_AVERAGED_XTALK_DELTA, 308},
	{VL53L1_TUNINGPARM_DYNXTALK_CLIP_LIMIT, 10240},
	{VL53L1_TUNINGPARM_DYNXTALK_XTALK_AMB_THRESHOLD, 128},
	{VL53L1_TUNINGPARM_DYNXTALK_NODETECT_SAMPLE_LIMIT, 40},
	{VL53L1_TUNINGPARM_DYNXTALK_NODETECT_XTALK_OFFSET_KCPS, 410}
};

#endif /* STMVL53L1_TUNINGS_H */
