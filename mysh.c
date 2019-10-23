#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h> 
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
//global variables used in program
int const ClineSize = 512; //command line size in characters 
int clt = 0; // command line tokens for creating array size for parsing command line
int jidCount = 0; //creates the count unique job id
//structure for organzing the jobs
struct Job {
	int jid; //unique job id(jid)
	char commandLine[512];
	pid_t pid;
	int back; //checks to see if its in the background
};

struct b {
	char line[512];
};

int redirection = 0;

struct Job History[32]; //32 the big is the num of backgrd jobs running
int Batchmode = 0;
//functions called in program
int parseCommandLine(char cLine[], char *cLineTokens[]);
void interactiveMode(FILE * fp, char userInput[ClineSize + 1]);
void batchMode(FILE * fp, char argv[]);
void stripString (char str[]); //strips strings of special chars at the end
void commandRun(char userInput[]); //runs command from user
int isDigit(char number[]);
void clearTokens(char userInput[], char *cLineTokens[]);

int main( int argc, char* argv[] )
{
    FILE * fp; //batch file to open
	char userInput[1024]; //for user command line input
	
  // checks for valid arguments from user
  if (argc > 2) {
	char msg[] = "Usage: mysh [batchFile]\n";
	write(STDERR_FILENO, msg, strlen(msg));
    exit(1);
  } 
  // checks for valid file
  if (argc == 2) {

		fp = fopen(argv[1], "r"); //opens batch file
		
		if (fp == NULL) {
			char msg[] = "Error: Cannot open file ";	
			strcat(msg,argv[1]);
			strcat(msg,"\n");
			write(STDERR_FILENO, msg, strlen(msg));
			exit(1);
		}
		Batchmode = 1;
		batchMode(fp, argv[1]); //runs batch mode
  } else {
	  //since its in interactive mode we want to get user input with stdin
	  fp = stdin;
	  interactiveMode(fp, userInput); //run interactive mode
  } 
   fclose(fp); 
  return 0;
}

int parseCommandLine(char cLine[], char *cLineTokens[]) {
    // Returns first token 
	char copy[strlen(cLine)];
	strcpy(copy, cLine);
    char* token = strtok(copy, " ");
    // Keep printing tokens while one of the delimiters present 
	int i = 0;
    while (token != NULL) {		
		cLineTokens[i] = token;	
        token = strtok(NULL, " ");	
		i++;
    }
	
	cLineTokens[i] = NULL;
    return i;
}

void interactiveMode(FILE * fp, char userInput[]){
	while(1){
	  //displays prompt to user
	  write(STDOUT_FILENO,"mysh> ", strlen("mysh> "));
	  
	  //checks to see if user wants to exit shell using ctrl + d
	  if (fgets(userInput,ClineSize + 1,fp) == NULL) {
		  write(STDOUT_FILENO, "\n", strlen("\n"));
		  exit(0);
	  }
	  //strips command of any weird ending chars from fgets
	  stripString(userInput);
	  
	  
	  
	  //handles redirection-----------------------------------
	  if (strchr(userInput, '>') != NULL){
			char copy[512];
			strcpy(copy, userInput);
			
			//gets command line 
			char* token = strtok(copy, ">");
			char cLine[21];
			strcpy(cLine, token);
			
			//gets filename to redirect to
			token = strtok(NULL, ">");
			char fileName[21];
			strcpy(fileName, token);
			char *FileTokens[10];
			parseCommandLine(fileName,FileTokens);
			
			printf("-----------------------&s-----------\n", FileTokens[0]);
			//redirects standards output and standard error to file
			if (0 == access(FileTokens[0], F_OK)) { 
				//file exist
			} 
			else {
				//file does not exist
				printf("-----------------------&s-----------\n", FileTokens[0]);
				FILE * newfile = fopen(FileTokens[0], "ab+");
				fclose(newfile);
				
			}
			
			
			//calls command
			//commandRun(cLine);
			
			//redirects back to to std out and error from file.
			
			
			
			
			return;
	  } else {
		  //handles command line if  greater than 512 characters
			if (strlen(userInput) > ClineSize) {
				write(STDERR_FILENO,"Error: Command line is longer than 512 characters",
				strlen("Error: Command line is longer than 512 characters"));
				continue;
			}
	  
			//handles empty command line
			if(strcmp (userInput, "\n") == 0){
				continue;
			}
			commandRun(userInput);
	  }
  }
}

void batchMode(FILE * fp, char argv[]){
	size_t len = 0;
	char * buffer = NULL;
	int file = getline(&buffer, &len, fp);
	
	int numOfLines = 0;
	
	
	struct b fileLines[1024]; 
	
	while(file != -1){
		strcpy(fileLines[numOfLines].line, buffer);
		stripString(fileLines[numOfLines].line);
		file = getline(&buffer, &len, fp);
		numOfLines++;
	}
	
	for (int i = 0; i < numOfLines; i++){
		//detects empty line
		if (strlen(fileLines[i].line) != 1) {
			//detects empty line
			write(STDOUT_FILENO,fileLines[i].line,strlen(fileLines[i].line));
			write(STDOUT_FILENO,"\n",strlen("\n"));
			commandRun(fileLines[i].line);
		}
	}
	
	return;
	
}

void stripString (char str[]) {
	int ln = strlen(str);
    // gets ride of trailing newline
	if (ln >=2){
		if (str[ln-2] == '\r') {
			str[ln-2] = '\0';
		}
		
		if (str[ln-1] == '\n') {
			str[ln-1] = '\0';
		}
			
	}
	return;
}

void commandRun(char userInput[]){
	int numOfWords;
	char *cLineTokens[512];
	clt = strlen(userInput) + 1;
	char msg[ClineSize + 7]; //handles msg output
	//parse the user input
	numOfWords = parseCommandLine(userInput, cLineTokens);
	
	//handles multiple white spaces on command line with enter
	if(numOfWords == 0 || cLineTokens[0] == NULL){
		return;
	}
	  //checks if user want to exit by typing exit
	  if (strcmp(cLineTokens[0],"exit") == 0){
		  exit(0);
	  }
	  //handles the command "jobs" (built-in shell)
	  if (strcmp(cLineTokens[0],"jobs") == 0){
			for (int i = 0; i < jidCount; i++){
				if (History[i].back == 0){
					continue;
				}
				//checks if job is still running 
				int status;
				if (waitpid(History[i].pid, &status, WNOHANG) == 0) {
					sprintf(msg, "%d: %s\n",History[i].jid, History[i].commandLine);
					write(STDOUT_FILENO, msg, strlen(msg));
					memset(msg, 0, strlen(msg));
				}
			}
		  return;
	  }
	  //checks for the wait command
	  if ((strcmp(cLineTokens[0],"wait") == 0)&& (numOfWords == 2) && (strcmp(cLineTokens[1],"&") != 0) ){
		  //checks to see if we have a digit for arg 2 of wait command
		  if (!isDigit(cLineTokens[1])) {
			if (atoi(cLineTokens[1]) > jidCount){
				sprintf(msg,"Invalid JID %s\n",cLineTokens[1]);
				write(STDERR_FILENO,msg,strlen(msg));
				
			}
			//handles waiting for foreground job
			if (History[atoi(cLineTokens[1])].back == 0) {
				sprintf(msg,"JID %s terminated",cLineTokens[1]);
				write(STDOUT_FILENO,msg,strlen(msg));
			} 
			//handles waiting for a background job
			if (History[atoi(cLineTokens[1])].back == 1) {
				//waits for job to be done
				int status;
				//fix me----------------------------------should be a do while loop
				waitpid(History[atoi(cLineTokens[1])].pid, &status,0);
				//prints when job is done
				sprintf(msg,"JID %s terminated",cLineTokens[1]);
				write(STDOUT_FILENO,msg,strlen(msg));
			}
		  
			memset(msg, 0, strlen(msg));
			return; 
		  }
	  }
	  //puts job in history structure array 
	  History[jidCount].jid = jidCount;
	  //adds tokens to commandline up until (num of words - 1)
	  for (int i = 0; i < numOfWords - 1; i++){
			strcat(History[jidCount].commandLine, cLineTokens[i]);
      }
	  //makes sure to set different values in struct between foreground and background in history
	  if (strcmp(cLineTokens[numOfWords-1],"&") == 0){
			cLineTokens[numOfWords-1] = NULL; //makes sure we dont pass it on to the process
			History[jidCount].back = 1;
	  } else {
			//finishes adding to commandline if its not in background mode
		     strcat(History[jidCount].commandLine, cLineTokens[numOfWords-1]);
			 History[jidCount].back = 0; 
	  }
	 
	 pid_t process = fork();
	 
	  //parent process 
	  if (process > 0) {
		 if(History[jidCount].back == 0){
			//process in foreground waits to finish before displaying prompt
			int childStatus;
			pid_t terminate;
			do {
			 terminate = wait(&childStatus);
			} while(terminate != process);
		 }
	  }
	  //child process
	  else if (process == 0){
		 
		 execvp(cLineTokens[0], cLineTokens);
		 
		 //if it returns must be invalid  job: Command not found
		 strcat(cLineTokens[0],": Command not found\n");
		 write(STDERR_FILENO, cLineTokens[0], strlen(cLineTokens[0]));
		 exit(1);
	  }
	  else {
		 exit(1); //creation of the child process was unsuccessful
	  }
	  
	  jidCount++;
	  History[jidCount].pid = process;
	  return;
}

int isDigit(char number[]){
	//took this from my across.c
	for (int i =0; i < strlen(number); i++){
		if(!isdigit(number[i])){
			return 0;
		}
	}
	return 1;
}

void clearTokens(char userInput[],char *cLineTokens[]){
	int i = 0;
    while (cLineTokens[i] != NULL) {	
		cLineTokens[i] = NULL;
		i++;
    }
	memset(userInput, 0, 1024);
    return;
}
