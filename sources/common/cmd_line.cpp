/**
 ******************************************************************************
 * @author: ThanNT
 * @date:   13/08/2016
 ******************************************************************************
 **/

#include <string.h>
#include "cmd_line.h"

static uint8_t str_list_len;
static char cmd_buffer[STR_BUFFER_SIZE];
static char *str_list[STR_LIST_MAX_SIZE];

uint8_t cmd_line_parser(cmd_line_t *cmd_table, uint8_t *command) {
	uint8_t cmd[MAX_CMD_SIZE];
	uint8_t *p_command = command;
	uint8_t cmd_index  = 0;
	uint8_t indexCheck = 0;

	if (cmd_table == (cmd_line_t *)0) {
		return CMD_TBL_NOT_FOUND;
	}

	/* get cmd */
	while (*p_command) {
		if (*p_command == ' ' || *p_command == '\r' || *p_command == '\n') {
			cmd[cmd_index] = 0;
			break;
		}
		else {
			cmd[cmd_index++] = *(p_command++);
			if (cmd_index >= MAX_CMD_SIZE) {
				return CMD_TOO_LONG;
			}
		}
	}

	/* find respective command in command table */
	while (cmd_table[indexCheck].cmd) {
		if (strcmp((const char *)cmd_table[indexCheck].cmd, (const char *)cmd) == 0) {
			/* perform respective function */
			cmd_table[indexCheck].func(command);

			/* return success */
			return CMD_SUCCESS;
		}

		indexCheck++;
	}

	return CMD_NOT_FOUND; /* command not found */
}

uint8_t cmd_str_parser(char *str, char c) {
	memset(cmd_buffer, 0, sizeof(char) * STR_BUFFER_SIZE);
	strcpy(cmd_buffer, str);
	str_list_len = 0;

	uint16_t i				= 0;
	uint8_t str_list_index	= 0;
	uint8_t flag_insert_str = 1;

	while (cmd_buffer[i] != 0 && cmd_buffer[i] != '\n' && cmd_buffer[i] != '\r') {
		if (cmd_buffer[i] == c) {
			cmd_buffer[i]	= 0;
			flag_insert_str = 1;
		}
		else if (flag_insert_str) {
			str_list[str_list_index++] = &cmd_buffer[i];
			flag_insert_str			   = 0;
		}
		i++;
	}

	cmd_buffer[i] = 0;

	str_list_len = str_list_index;
	return str_list_len;
}

char *cmd_str_parser_get_attr(uint8_t index) {
	if (index < str_list_len) {
		return str_list[index];
	}

	return NULL;
}