#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

#define MAX_PATH  4096

char prompt[20] = "MY_Shell$";
typedef struct EnvVar {
	char* name;
	char* value;
	struct EnvVar* next;
}EnvVar;
EnvVar* env_list = NULL;
int fd;
char lastDir[MAX_PATH] = {0};
int i = 1;
void chprompt_command(const char* new_prompt) {
    if (new_prompt != NULL && strlen(new_prompt) > 0) {
         snprintf(prompt, sizeof(prompt), "%s", new_prompt);
         printf("Prompt changed to: %s\n", prompt);
    }else {
        printf("prompt failed\n");
    }
}

void history_command(const char* input) {

	snprintf(lastDir, sizeof(lastDir), "   %d    %s\n",i, input);
	++i;
	write(fd, lastDir, strlen(lastDir));
}

void help_command(void) {
	printf("Mini Shell - Built-in Commands:\n");
	printf(" cd <dir>        Change directory to <dir>\n");
	printf(" exit            Exit the shell\n");
	printf(" help            Show this help message\n");
	printf(" clear           Clear the terminal screen\n");
	printf(" pwd             Print the current working directory\n"); 
	printf(" echo <text>     Print <text> to the terminal\n");
	printf(" setenv          Description: Sets or modifies an environment variable. \n");
	printf(" unsetenv        Description: Removes an environment variable. \n");
	printf(" chprompt        Description: Changes the command prompt to the specified string. \n");
	printf(" chprompt <new_prompt>  Change the shell prompt to <new_prompt>\n");
	printf("Redirection:\n");
	printf(" < file          Redirect input from <file>\n");
	printf(" > fil           Redirect output to <file>\n");
}

void exit_command(void) {
	printf("Exit is shel\n");
	close(fd);
	exit(0);
}

void clear_command(void) {
	printf("\033[H\033[J");
}

void cd_command(const char* dir) {
	if (chdir(dir) != 0) {
		perror("cd filed");
	}
}

void pwd_command(void) {
	long n; 
	char cwd[MAX_PATH];
	if (getcwd(cwd, sizeof(cwd)) != NULL) {
		printf("%s\n",cwd);
	}else {
		printf("pwd filed");
	}
}
EnvVar* find_env_var(const char* var_name);
// Print the provided text to the terminal
void echo_command(char* text) {
    char* var_start = NULL;
    char* var_end = NULL;
    char var_name[MAX_PATH];
    char* result = text;
    char expanded[MAX_PATH];
    int i = 0, j = 0;

    while ((var_start = strchr(result, '$')) != NULL) {
        // Copy everything before the variable
        while (result < var_start) {
            expanded[j++] = *result++;
        }
        result = var_start + 1;  // Skip the '$' symbol

        // Find the end of the variable name
        var_end = strpbrk(result, " \t\r\n$");
        if (var_end) {
            strncpy(var_name, result, var_end - result);
            var_name[var_end - result] = '\0';
            result = var_end;
        } else {
            // No more characters, end of string
            strcpy(var_name, result);
            result = "";
        }

        // Check if the variable exists in the environment list
        EnvVar* var = find_env_var(var_name);
        if (var != NULL) {
            // Replace the variable with its value
            strcpy(&expanded[j], var->value);
            j += strlen(var->value);
        } else {
            // If variable not found, leave it as-is
            expanded[j++] = '$';
            strcpy(&expanded[j], var_name);
            j += strlen(var_name);
        }
    }

    // Copy the remaining part of the text
    while (*result != '\0') {
        expanded[j++] = *result++;
    }

    expanded[j] = '\0';
    printf("%s\n", expanded);
}


EnvVar* find_env_var(const char* var_name) {
	EnvVar* current = env_list;
	while (current != NULL) {
		if(strcmp(current->name, var_name) == 0) {
			return current;
		}
		current = current->next;
	}
	return NULL;
}

void set_env_var (const char* var_name, const char* value) {
	EnvVar* existing_var = find_env_var(var_name);

	if (existing_var != NULL) {
		free(existing_var->value);
		existing_var->value = strdup(value);
		printf("Envairoment variable '%s' update to '%s'\n", var_name, value);
	}else {
		EnvVar* new_var = (EnvVar*)malloc (sizeof(EnvVar));
		new_var->name = strdup(var_name);
		new_var->value = strdup(value);
		new_var->next = env_list;
		env_list = new_var;
		printf("Environment variable '%s' set to '%s'\n", var_name, value);
	}
}


void print_env_vars() {
	EnvVar* current = env_list;
	while (current != NULL) {
		printf("%s=%s\n", current->name,current->value);
		current = current->next;
	}
}

void setenv_command(const char* var_name, const char* value) {
	if(var_name == NULL || value == NULL || strlen(var_name) == 0 || strlen(value) == 0) {
		printf("Usage: setenv <variable_name> <value>\n");
		return;
	}
	set_env_var(var_name, value);
}
void unsetevn_command(const char* var_name) {
        if (var_name == NULL && strlen(var_name) == 0) {
            printf("Usage: unsetenv <variable_name>\n");
            return;
        }
		EnvVar* current = env_list;
		EnvVar* prev = NULL;
		while (current != NULL)
		{
			if (strcmp(current->name, var_name) == 0)
			{
		
				if (prev == NULL)
				{
					env_list = current->next;

				}else {
					prev->next = current->next;
				}
				free(current->name);
				free(current->value);
				free(current);

				return;
			}
			prev = current;
			current = current->next;
			
		}
		 printf("Environment variable '%s' not \n", var_name);
		
}
void command_history() {
	int bytes_read;
	char buff[200] = {0};
	lseek(fd, 0, SEEK_SET);	
	bytes_read = read(fd, buff, sizeof(buff) - 1);
	if (bytes_read == -1) {
		printf("No command\n");
	}
	//buff[bytes_read] = '\0';
	printf("%s\n", buff);
}


void command (char* arg) {
	history_command (arg);
	
	if (strcmp(arg, "help") == 0) {
		help_command();
	}else if (strcmp(arg, "exit") == 0) {
		exit_command();
	}else if (strcmp(arg, "clear") == 0) {
		clear_command();
	}else if (strncmp(arg, "cd", 2) == 0) {
		char* dir = arg + 3;
		if (strlen(dir) == 0) {
			dir = getenv("HOME");		
		}
		cd_command(dir);
	}else if (strcmp(arg, "pwd") == 0) {
		pwd_command();
	}else if (strncmp(arg, "echo", 4) == 0) {
		char* text = arg + 5;
		if (strlen(text) > 0) {
			echo_command(text);
		}else {
			printf("\n");
		}
	}else if (strncmp(arg, "setenv", 6) == 0) {
		char* var_name = arg + 7;
		char* value = strchr(var_name, ' ');
		if (value != NULL) {
			*value = '\0';
			value++;
			while (*value == ' ') {
				value++;
			}
			if (strlen(value) > 0 && strlen(var_name) > 0) {
				setenv_command(var_name, value);
			}else {
				printf("Usage: setenv <variable_name> <value>\n");
			}
		}else {
			printf("Usage: setenv <name> <value>\n");
		}
		
	}else if (strncmp(arg, "unsetenv", 8) == 0)
	{
		char *name = arg + 9;
		while (*name == ' ')
		{
			name++;
		}
		if (strlen(name) > 0)
		{
			unsetevn_command(name);
		}else {
			printf("Useg: unsetenv <name>/n");
		}
		
		
		
	}else if (strcmp(arg, "history") == 0) {
		command_history();
		
	}else if (strncmp(arg,"chprompt",8) == 0) {
        char* new_prompt = arg + 9;
        while (*new_prompt == ' ') new_prompt++;
        if (strlen(new_prompt) > 0) {
            chprompt_command(new_prompt);
        }
	}	
}



int main () {
	char input[MAX_PATH];
	fd = open("history.txt", O_TRUNC | O_CREAT | O_RDWR | O_APPEND , 0777 );	
	if (fd == -1) {
		perror("Error opening file");
		return 1;
	}
	while (1) {
		printf("%s ", prompt);
		if (fgets(input, sizeof(input), stdin) == NULL) {
			printf("\n");
			break;
		}
		input[strlen(input) - 1] = '\0';
		command(input);
	}
	return 0;
}
