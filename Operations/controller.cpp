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

#include "Operations.hpp"

static shared_timed_mutex GlobalLock;

int AMSD::Operations::controller(json_t *in_data, json_t *&out_data){
	json_t *j_op = json_object_get(in_data, "op");
	string op;
	size_t index = 0;
	json_t *j_controllers, *j_controller;
	json_t *j_con_ip, *j_con_port;
	json_t *j_errmsg;
	sqlite3_stmt *stmt;
	int rc;
	const void *addr;
	int addr_len;
	char addrsbuf[INET6_ADDRSTRLEN];
	time_t timenow;

	if (!j_op || !json_is_string(j_op))
		return -1;

	op = json_string_value(j_op);

	try {
		Reimu::SQLAutomator::SQLite3 *thisdb = db_controller.OpenSQLite3();

		if (op == "list") {

			j_controllers = json_array();

			thisdb->Prepare(db_controller.Statement(SELECT_FROM));

			while ( (rc = thisdb->Step()) == SQLITE_ROW) {
				j_controller = json_object();

				addr = sqlite3_column_blob(thisdb->SQLite3Statement, 1);
				addr_len = sqlite3_column_bytes(thisdb->SQLite3Statement, 1);

				if (addr_len == 4) {
					inet_ntop(AF_INET, addr, addrsbuf, INET_ADDRSTRLEN);
				} else {
					inet_ntop(AF_INET6, addr, addrsbuf, INET6_ADDRSTRLEN);
				}

				json_object_set_new(j_controller, "mtime", json_integer(sqlite3_column_int(thisdb->SQLite3Statement, 0)));
				json_object_set_new(j_controller, "ip", json_string(addrsbuf));
				json_object_set_new(j_controller, "port", json_integer(sqlite3_column_int(thisdb->SQLite3Statement, 2)));
				json_array_append_new(j_controllers, j_controller);
			}


			json_object_set_new(out_data, "controllers", j_controllers);

		} else if (op == "add") {


			j_controllers = json_object_get(in_data, "controllers");
			if (!j_controllers || !json_is_array(j_controllers))
				return -1;

			timenow = time(NULL);

			GlobalLock.lock();
			thisdb->Exec("BEGIN");

			thisdb->Prepare("INSERT INTO controller VALUES (?1, ?2, ?3, 7)");

			json_array_foreach(j_controllers, index, j_controller) {
				if (json_is_object(j_controller)) {
					j_con_ip = json_object_get(j_controller, "ip");
					j_con_port = json_object_get(j_controller, "port");

					if (j_con_ip && j_con_port)
						if (json_is_string(j_con_ip) && json_is_integer(j_con_port)) {

							json_int_t thisport = json_integer_value(j_con_port);

							thisdb->Bind(1, timenow);
							thisdb->Bind(3, (int64_t)thisport);


							const char *ipsbuf = json_string_value(j_con_ip);

							Reimu::IPEndPoint remoteEP(string(ipsbuf), (uint16_t)thisport);

							thisdb->Bind(2, {remoteEP.Addr,
									remoteEP.AddressFamily == AF_INET ? 4 : 16});


							thisdb->Step();
							thisdb->Reset();
						}
				}
			}

			thisdb->Exec("COMMIT");

			GlobalLock.unlock();
		} else if (op == "del") {
			j_controllers = json_object_get(in_data, "controllers");
			if (!j_controllers || !json_is_array(j_controllers))
				return -1;

			GlobalLock.lock();
			thisdb->Exec("BEGIN");
			thisdb->Prepare("DELETE FROM controller WHERE Addr = ?1 AND Port = ?2");

			json_array_foreach(j_controllers, index, j_controller) {
				if (json_is_object(j_controller)) {
					j_con_ip = json_object_get(j_controller, "ip");
					j_con_port = json_object_get(j_controller, "port");

					if (j_con_ip && j_con_port)
						if (json_is_string(j_con_ip) && json_is_integer(j_con_port)) {

							const char *ipsbuf = json_string_value(j_con_ip);

							if (strchr(ipsbuf, ':')) {
								in6_addr thisaddr;
								inet_pton(AF_INET6, ipsbuf, &thisaddr);
								thisdb->Bind(1, {thisaddr.__in6_u.__u6_addr8, 16});
							} else {
								in_addr thisaddr;
								inet_pton(AF_INET, ipsbuf, &thisaddr);
								thisdb->Bind(1, {&thisaddr.s_addr, 4});
							}

							thisdb->Bind(2, {(int64_t)json_integer_value(j_con_port)});

							thisdb->Step();
							thisdb->Reset();
						}
				}
			}

			thisdb->Exec("COMMIT");
			GlobalLock.unlock();

		} else if (op == "wipe") {
			GlobalLock.lock();
			thisdb->Exec("TRUNCATE controller");
			GlobalLock.unlock();
		}

		delete thisdb;

	} catch (Reimu::Exception e) {
		return -2;
	}

	return 0;
}