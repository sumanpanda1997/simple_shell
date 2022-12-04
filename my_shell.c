#include  <stdio.h>
#include  <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64

int interrupt = 0;

/* Splits the string by space and returns the array of tokens
*
*/
char **tokenize(char *line)
{
  char **tokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
  char *token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
  int i, tokenIndex = 0, tokenNo = 0;

  for(i =0; i < strlen(line); i++){

    char readChar = line[i];

    if (readChar == ' ' || readChar == '\n' || readChar == '\t'){
      token[tokenIndex] = '\0';
      if (tokenIndex != 0){
	tokens[tokenNo] = (char*)malloc(MAX_TOKEN_SIZE*sizeof(char));
	strcpy(tokens[tokenNo++], token);
	tokenIndex = 0; 
      }
    } else {
      token[tokenIndex++] = readChar;
    }
  }
 
  free(token);
  tokens[tokenNo] = NULL ;
  return tokens;
}

void free_token(char** tokens)
{
	int i;
	for(i=0;tokens[i]!=NULL;i++){
		free(tokens[i]);
	}
	free(tokens);
	return;
}

void sigint_handler(int sigint)
{	
	interrupt = 1;
	return;
}


int main(int argc, char* argv[]) {

	signal(SIGINT, sigint_handler);
	char  line[MAX_INPUT_SIZE];            
	char  **tokens;              
	int i;

	int bg_counter = 0;
	int bg_flag = 0;
	int bg_pid_array[64];
	for(int i=0; i<64; i++)
	{
		bg_pid_array[i] = 0;
	} 


	while(1) {			

		/* BEGIN: TAKING INPUT */
		bzero(line, sizeof(line));
		printf("$ ");
		scanf("%[^\n]", line);
		//printf("%s\n", line);
		getchar();
		if(interrupt)
		{
			interrupt = 0;
			continue;
		}

		//checks for back ground process termination and if terminated then reaps them
		bg_flag = 0;
		if(bg_counter > 0)
		{
			//printf("there are %d process in back ground\n", bg_counter);
			//reap all possible child which are zombies
			while(1)
			{
				int pid = waitpid(-1, NULL, WNOHANG);
				if(pid > 0)
				{
					printf("Shell: Background process finished pid = %d\n", pid);
					int itr = 0;
					//clearing the bg pid array for that corresponding pid which returned success from waitpid
					while(itr < 64)
					{
						if(bg_pid_array[itr] == pid)
						{
							bg_pid_array[itr] = 0;
							break;
						}
						itr++;
					}
					bg_counter--;
				}
				else
					break;			
			}
			if(bg_counter == 0)
			{
				//printf("no back ground process\n");
			}
		}

		//printf("Command entered: %s (remove this debug output later)\n", line);
		/* END: TAKING INPUT */

		line[strlen(line)] = '\n'; //terminate with new line
		tokens = tokenize(line);
   
	   int token_count = 0;
		for(i=0;tokens[i]!=NULL;i++){
			//printf("found token %s (remove this debug output later)\n", tokens[i]);
			token_count++;
		}

		//handling empty return logic
		if(tokens[0] == NULL)
			continue;

		//handling cd logic
		if(strcmp(tokens[0], "cd") == 0)
		{
			if(token_count!=2 || chdir(tokens[1]) != 0)
				fprintf(stderr, "%s\n", "Shell: Incorrect command");
			free_token(tokens);
			continue;
		}

		//handling exit logic
		if(strcmp(tokens[0], "exit") == 0)
		{
			if(token_count != 1)
			{
				fprintf(stderr, "%s\n", "shell: inocrrect command");
				free_token(tokens);
				continue;
			}

			int itr = 0;
			while(itr < 64)
			{
				int pid = bg_pid_array[itr];
				if(pid > 0)
				{
					//printf("clearing pid = %d\n", pid);
					kill(pid, SIGTERM);
					int pid_status = waitpid(pid, NULL, 0);

					if(pid_status > 0)
						printf("Shell: Background process finished pid = %d\n", pid_status);
				}
				itr++;
			}
			free_token(tokens);
			exit(0);
		}
		
		//handling background logic
		if( strcmp(tokens[token_count-1], "&") == 0)
		{
			bg_flag = 1;
			tokens[token_count-1] = NULL;
		}
	
	   int fc = fork();
	   if(fc < 0)
	   {
			fprintf(stderr, "%s\n", "child fork failed");
	   }
	   else if(fc == 0)
	   {

			if(bg_flag)
				setpgid(0, 0);
			execvp(tokens[0], tokens);
			printf("command not found in system executable\n");
			_exit(1);
	   }
	   else
	   {
			if(bg_flag == 0)
			{
				int wc = waitpid(fc, NULL, 0);
				if(interrupt)
				{
					printf("\n");
					interrupt = 0;
				}
			}
			else
			{
				int itr = 0;
				while(itr < 64)
				{	
					if(bg_pid_array[itr] == 0)
					{
						bg_pid_array[itr] = fc;
						break;
					}
					itr++;
				}
				bg_counter++;
			}
	   }
       
		// Freeing the allocated memory	
		free_token(tokens);

	}
	return 0;
}
