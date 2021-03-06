/*
    This file is part of AMSD.
    Copyright (C) 2016-2017  CloudyReimu <cloudyreimu@gmail.com>

    AMSD is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    AMSD is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with AMSD.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "../amsd.hpp"

#define AMSD_NVRAM_SIZE			32


static uint8_t *RuntimeData_Memory = NULL;

static const char Current_NVRAM_Version[] = "AMSD NVRAM REV 1.000";

uint8_t *AMSD::RuntimeData::RuntimeData_Memory_Token = NULL;

static uint8_t *RDM_OFFSET_NVRAM_Version;

static uint8_t *RDM_OFFSET_TimeStamp_CurrentDataCollection;
static uint8_t *RDM_OFFSET_TimeStamp_LastDataCollection;
static uint8_t *RDM_OFFSET_TimeStamp_DBFirstDataCollection;

Kanna::Logging AMSD_Logger("/var/log/amsd.log");

int AMSD::RuntimeData::Init() {
	if (!Reimu::FileSystem::Exist(Config::Path_RuntimeDir)) {
		cerr << "amsd: RuntimeData::Init: error: faile to open directory " << Config::Path_RuntimeDir << "\n";
		return -1;
	}
	string shm_path = Config::Path_RuntimeDir + "/NVRAM";
	RuntimeData_Memory = reimu_shm_open(shm_path, AMSD_NVRAM_SIZE*1024);
	if (!RuntimeData_Memory)
		return -2;

	cerr << "amsd: RuntimeData: " << AMSD_NVRAM_SIZE << "KB NVRAM present\n";

	RDM_OFFSET_NVRAM_Version = RuntimeData_Memory;

	if (RDM_OFFSET_NVRAM_Version[0] == 0) {
		strcpy((char *)RDM_OFFSET_NVRAM_Version, Current_NVRAM_Version);
		msync(RDM_OFFSET_NVRAM_Version, 24, MS_SYNC);
	} else {
		if (strcmp((char *)RDM_OFFSET_NVRAM_Version, Current_NVRAM_Version)) {
			cerr << "amsd: RuntimeData: Warning: NVRAM version mismatch!! Please fix.\n";
			abort();
		}
	}

	RDM_OFFSET_TimeStamp_CurrentDataCollection = RuntimeData_Memory + 128;
	RDM_OFFSET_TimeStamp_LastDataCollection = RDM_OFFSET_TimeStamp_CurrentDataCollection + 8;
	RDM_OFFSET_TimeStamp_DBFirstDataCollection = RDM_OFFSET_TimeStamp_LastDataCollection + 8;



	shm_path = Config::Path_RuntimeDir + "/shm";


	AMSD::RuntimeData::RuntimeData_Memory_Token = reimu_shm_open(shm_path, 8192, 1);

	if (!AMSD::RuntimeData::RuntimeData_Memory_Token) {
		fprintf(stderr,"amsd: reimu_shm_open() failed\n");
		return 2;
	}

	fprintf(stderr,"amsd: 8KB shared memory at %p\n", (void *)AMSD::RuntimeData::RuntimeData_Memory_Token);

	snprintf((char *)AMSD::RuntimeData::RuntimeData_Memory_Token, 256, "%s", AMSD::User::LocalAdminToken.c_str());




	return 0;
}

time_t AMSD::RuntimeData::TimeStamp::CurrentDataCollection() {
	time_t *pRD = (time_t *)RDM_OFFSET_TimeStamp_CurrentDataCollection;
	return *pRD;
}

void AMSD::RuntimeData::TimeStamp::CurrentDataCollection(time_t t) {
	time_t *pRD = (time_t *)RDM_OFFSET_TimeStamp_CurrentDataCollection;
	*pRD = t;
}

time_t AMSD::RuntimeData::TimeStamp::LastDataCollection() {
	time_t *pRD = (time_t *)RDM_OFFSET_TimeStamp_LastDataCollection;
	return *pRD;
}


void AMSD::RuntimeData::TimeStamp::LastDataCollection(time_t t) {
	time_t *pRD = (time_t *)RDM_OFFSET_TimeStamp_LastDataCollection;
	*pRD = t;
}

