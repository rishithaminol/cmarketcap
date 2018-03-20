#include <stdio.h>
#include <my_global.h>
#include <mysql.h>

#include "cm_debug.h"
#include "cmarketcap.h"

char *prog_name = "mysql_api_test";

int main(int argc, char **argv)
{
	MYSQL *con = mysql_init(NULL);

	if (con == NULL) {
		CM_ERROR("%s\n", mysql_error(con));
		exit(1);
	}

	if (mysql_real_connect(con, "localhost", "root", "rootroot01",
	  NULL, 0, NULL, 0) == NULL)
	{
		CM_ERROR("%s\n", mysql_error(con));
		mysql_close(con);
		exit(1);
	}

	if (mysql_query(con, "CREATE DATABASE testdb")) {
		CM_ERROR("%s\n", mysql_error(con));
		mysql_close(con);
		exit(1);
	}

	mysql_close(con);

	return 0;
}
