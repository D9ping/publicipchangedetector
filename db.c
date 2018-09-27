/***************************************************************************
 *    Copyright (C) 2018 D9ping
 *
 * PublicIpChangeDetector is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PublicIpChangeDetector is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PublicIpChangeDetector. If not, see <http://www.gnu.org/licenses/>.
 *
 ***************************************************************************/
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <sqlite3.h>

#define MAXLENURL          1023
#define MAXLENCONFIGSTR    255

int create_table_ipservice(sqlite3 *db, bool verbosemode)
{
        int retcode;
        sqlite3_stmt *stmt = NULL;
        sqlite3_prepare_v2(db, "CREATE TABLE IF NOT EXISTS `ipservice` ( \
 `nr` INT PRIMARY KEY NOT NULL, \
 `disabled` TINYINT NOT NULL DEFAULT 0, \
 `type` TINYINT NOT NULL, \
 `url` TEXT(1023) NOT NULL, \
 `lastErrorOn` NUMERIC );", -1, &stmt, NULL);
        retcode = sqlite3_step(stmt);
        if (retcode != SQLITE_DONE) {
                fprintf(stderr, "Error creating ipservice table: %s\n", sqlite3_errmsg(db));
        } else if (verbosemode) {
                fprintf(stdout, "Table ipservice succesfully created.\n");
        }

        sqlite3_finalize(stmt);
        return retcode;
}


/**
        Add a ipservice to the ipservice database table.
        ipservice:
        if disabled is true then the ipservice has a pernament error and should not be used.
        type = 0 for dns(not supported), type = 1 for http, type = 2 for https
 */
int add_ipservice(sqlite3 *db, int urlnr, char * url, bool disabled, int type, bool verbosemode)
{
        if (type < 0 || type > 2) {
                exit(EXIT_FAILURE);
        }

        if (strlen(url) >= MAXLENURL) {
            printf("Url too long, maximum %d character allowed.\n", MAXLENURL);
            return 1;
        }

        int retcode;
        sqlite3_stmt *stmt = NULL;
        sqlite3_prepare_v2(db, "INSERT INTO `ipservice` (`nr`, `disabled`, `type`, `url`)\
 VALUES (?1, ?2, ?3, ?4);", -1, &stmt, NULL);
        sqlite3_bind_int(stmt, 1, urlnr);
        sqlite3_bind_int(stmt, 2, disabled);
        sqlite3_bind_int(stmt, 3, type);
        sqlite3_bind_text(stmt, 4, url, -1, SQLITE_STATIC);
        retcode = sqlite3_step(stmt);
        if (retcode != SQLITE_DONE) {
                printf("ERROR inserting data: %s\n", sqlite3_errmsg(db));
        } else if (verbosemode) {
                fprintf(stdout, "Added ipservice record.\n");
        }

        sqlite3_finalize(stmt);
        return retcode;
}


/**
        Get the number of ipservices in the ipservice table.
*/
int get_count_all_ipservices(sqlite3 *db)
{
        int cntall = 0;
        sqlite3_stmt *stmt = NULL;
        sqlite3_prepare_v2(db,
                           "SELECT COUNT(`nr`) FROM `ipservice` LIMIT 1;",
                           1023,
                           &stmt,
                           NULL);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
                cntall = sqlite3_column_int(stmt, 0);
        }

        sqlite3_finalize(stmt);
        return cntall;
}


int get_count_available_ipservices(sqlite3 *db, int allowedprotocoltypes)
{
        int cntavailable = 0;
        sqlite3_stmt *stmt = NULL;
        sqlite3_prepare_v2(db,
                           "SELECT COUNT(`nr`) FROM `ipservice` WHERE `disabled` = 0 AND type >= ?1 LIMIT 1;",
                           -1,
                           &stmt,
                           NULL);
        sqlite3_bind_int(stmt, 1, allowedprotocoltypes);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
                cntavailable = sqlite3_column_int(stmt, 0);
        }

        sqlite3_finalize(stmt);
        return cntavailable;
}


/**
        Get the url for a ipservice number.
*/
const char * get_url_ipservice(sqlite3 *db, int urlnr)
{
        const char *url;
        char *urlipservice;
        int sizeurlipservice = MAXLENURL * sizeof(char);
        urlipservice = malloc(sizeurlipservice);
        sqlite3_stmt *stmt = NULL;
        sqlite3_prepare_v2(db,
                           "SELECT `url` FROM `ipservice` WHERE `nr` = ?1 LIMIT 1;",
                           sizeurlipservice,
                           &stmt,
                           NULL);
        sqlite3_bind_int(stmt, 1, urlnr);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
                url = sqlite3_column_text(stmt, 0);
        } else {
                url = "";
        }

        strcpy(urlipservice, url);
        sqlite3_finalize(stmt);
        return urlipservice;
}


int update_disabled_ipsevice(sqlite3 *db, int urlnr, bool addtimestamp)
{
        int retcode = 0;
        sqlite3_stmt *stmt = NULL;
        if (addtimestamp) {
                int timestampnow = (int)time(NULL);
                sqlite3_prepare_v2(db,
                                    "UPDATE `ipservice` SET `disabled`= 1, `lasterroron`= ?1\
 WHERE `nr`= ?2 LIMIT 1;",
                                   -1,
                                   &stmt,
                                   NULL);
                sqlite3_bind_int(stmt, 1, timestampnow);
                sqlite3_bind_int(stmt, 2, urlnr);
        } else {
               sqlite3_prepare_v2(db,
                                  "UPDATE `ipservice` SET `disabled`= 1 WHERE `nr`= ?2 LIMIT 1;",
                                  -1,
                                  &stmt,
                                  NULL);
               sqlite3_bind_int(stmt, 2, urlnr);
        }

        retcode = sqlite3_step(stmt);
        if (retcode != SQLITE_DONE) {
                fprintf(stderr, "Error updating ipservice: %s\n", sqlite3_errmsg(db));
        }

        sqlite3_finalize(stmt);
        return retcode;
}


void get_urlnrs_ipservices(sqlite3 *db, int urlnrs[], int disabled, int allowedprotocoltypes)
{
        int i = 0;
        sqlite3_stmt *stmt = NULL;
        sqlite3_prepare_v2(db,
                            "SELECT `nr` FROM `ipservice` WHERE `disabled` = ?1 AND type >= ?2;",
                            -1,
                            &stmt,
                            NULL);
        sqlite3_bind_int(stmt, 1, disabled);
        sqlite3_bind_int(stmt, 2, allowedprotocoltypes);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
                int nr;
                nr = sqlite3_column_int(stmt, 0);
                urlnrs[i] = nr;
                ++i;
        }

        sqlite3_finalize(stmt);
}


/**
        Create config table.
 */
int create_table_config(sqlite3 *db, bool verbosemode)
{
        int retcode;
        sqlite3_stmt *stmt = NULL;
        sqlite3_prepare_v2(db, "CREATE TABLE IF NOT EXISTS `config` ( \
 `name` TEXT, \
 `valueint` INT, \
 `valuestr` TEXT(255) );", -1, &stmt, NULL);
        retcode = sqlite3_step(stmt);
        if (retcode != SQLITE_DONE) {
                fprintf(stderr, "Error creating config table: %s\n", sqlite3_errmsg(db));
        } else if (verbosemode) {
                fprintf(stdout, "Table config succesfully created.\n");
        }

        sqlite3_finalize(stmt);
        return retcode;
}


int get_config_value_int(sqlite3 *db, char * name)
{
        int value_int = -1;
        int retcode = 0;
        sqlite3_stmt *stmt = NULL;
        retcode = sqlite3_prepare_v2(db,
                                     "SELECT `valueint` FROM `config` WHERE `name` = ?1 LIMIT 1;",
                                     -1,
                                     &stmt,
                                     NULL);
        sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
                value_int = sqlite3_column_int(stmt, 0);
                break;
        }

        sqlite3_finalize(stmt);
        return value_int;
}


char * get_config_value_str(sqlite3 *db, char * name)
{
        int configstrlen = MAXLENCONFIGSTR * sizeof(char);
        char *configvaluestr;
        configvaluestr = malloc(configstrlen);
        const char * value_str;
        sqlite3_stmt *stmt = NULL;
        sqlite3_prepare_v2(db,
                           "SELECT `valuestr` FROM `config` \
 WHERE `name` = ?1 LIMIT 1;",
                           -1,
                           &stmt,
                           NULL);
        sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
                value_str = sqlite3_column_text(stmt, 0);
                break;
        }

        strcpy(configvaluestr, value_str);
        sqlite3_finalize(stmt);
        return configvaluestr;
}


int add_config_value_int(sqlite3 *db, char * name, int value, bool verbosemode)
{
        int retcode;
        sqlite3_stmt *stmt = NULL;
        sqlite3_prepare_v2(db, "INSERT INTO `config` (`name`, `valueint`) \
 VALUES (?1, ?2);", -1, &stmt, NULL);
        sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 2, value);
        retcode = sqlite3_step(stmt);
        if (retcode != SQLITE_DONE) {
                fprintf(stderr, "Error storing config value: %s\n", sqlite3_errmsg(db));
        } else if (verbosemode) {
                fprintf(stdout, "Config: %s = %d  saved.\n", name, value);
        }

        sqlite3_finalize(stmt);
        return retcode;
}


int add_config_value_str(sqlite3 *db, char *name, char * value, bool verbosemode)
{
        int retcode;
        sqlite3_stmt *stmt = NULL;
        sqlite3_prepare_v2(db, "INSERT INTO `config` (`name`, `valuestr`) \
 VALUES (?1, ?2);", -1, &stmt, NULL);
        sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, value, -1, SQLITE_STATIC);
        retcode = sqlite3_step(stmt);
        if (retcode != SQLITE_DONE) {
                fprintf(stderr, "Error storing config value: %s\n", sqlite3_errmsg(db));
        } else if (verbosemode) {
                fprintf(stdout, "Config: %s = %s  saved.\n", name, value);
        }

        sqlite3_finalize(stmt);
        return retcode;
}


int update_config_value_int(sqlite3 *db, char * name, int valuenew, bool verbosemode)
{
        int retcode = 0;
        sqlite3_stmt *stmt = NULL;
        sqlite3_prepare_v2(db,
                           "UPDATE `config` SET `valueint`= ?1\
 WHERE  `name`= ?2 LIMIT 1;",
                          -1,
                          &stmt,
                          NULL);
        sqlite3_bind_int(stmt, 1, valuenew);
        sqlite3_bind_text(stmt, 2, name, -1, SQLITE_STATIC);
        retcode = sqlite3_step(stmt);
        if (retcode != SQLITE_DONE) {
                fprintf(stderr, "Error storing config value: %s\n", sqlite3_errmsg(db));
        } else if (verbosemode) {
                fprintf(stdout, "Config: %s = %d  saved.\n", name, valuenew);
        }

        sqlite3_finalize(stmt);
        return retcode;
}


int update_config_value_str(sqlite3 *db, char * name, const char * valuenew, bool verbosemode)
{
        if (strlen(valuenew) > MAXLENCONFIGSTR) {
                return -1;
        }

        int retcode;
        sqlite3_stmt *stmt = NULL;
        sqlite3_prepare_v2(db,
                           "UPDATE `config` SET `valuestr`= ?1\
 WHERE `name`= ?2 LIMIT 1;",
                           -1,
                           &stmt,
                           NULL);
        sqlite3_bind_text(stmt, 1, valuenew, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, name, -1, SQLITE_STATIC);
        retcode = sqlite3_step(stmt);
        if (retcode != SQLITE_DONE) {
                fprintf(stderr, "Error storing config value: %s\n", sqlite3_errmsg(db));
        } else if (verbosemode) {
                fprintf(stdout, "Config: %s = %s  saved.\n", name, valuenew);
        }

        sqlite3_finalize(stmt);
        return retcode;
}


bool is_config_exists(sqlite3 *db, char *name)
{
        int value_int = 0;
        sqlite3_stmt *stmt = NULL;
        sqlite3_prepare_v2(db,
                           "SELECT COUNT(`name`) FROM `config` \
 WHERE `name` = ?1 LIMIT 1;",
                           -1,
                           &stmt,
                           NULL);
        sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
                value_int = sqlite3_column_int(stmt, 0);
                break;
        }

        sqlite3_finalize(stmt);
        if (value_int >= 1) {
            return true;
        } else {
            return false;
        }
}
