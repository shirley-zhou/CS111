//test cases before making tar file, in make dist

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>

void sig_handler(int signum)
{
	fprintf(stderr, "%d caught\n", signum);
	//char* buff = 
	//write(stderr, )
	exit(signum);
}

int parse_flags(const char* oflag)
{
	if (strcmp(oflag, "append") == 0)
		return O_APPEND;
	if (strcmp(oflag, "cloexec") == 0)
		return O_CLOEXEC;
	if (strcmp(oflag, "creat") == 0)
		return O_CREAT;
	if (strcmp(oflag, "directory") == 0)
		return O_DIRECTORY;
	if (strcmp(oflag, "dsync") == 0)
		return O_DSYNC;
	if (strcmp(oflag, "excl") == 0)
		return O_EXCL;
	if (strcmp(oflag, "nofollow") == 0)
		return O_NOFOLLOW;
	if (strcmp(oflag, "nonblock") == 0)
		return O_NONBLOCK;
	if (strcmp(oflag, "rsync") == 0)
		return O_RSYNC;
	if (strcmp(oflag, "sync") == 0)
		return O_SYNC;
	if (strcmp(oflag, "trunc") == 0)
		return O_TRUNC;
	if (strcmp(oflag, "rdonly") == 0)
		return O_RDONLY;
	if (strcmp(oflag, "rdwr") == 0)
		return O_RDWR;
	if (strcmp(oflag, "wronly") == 0)
		return O_WRONLY;
	else
		return -1;
}

double timediff(struct timeval t1, struct timeval t2)
{
	long sec, usec;

	usec = t2.tv_usec - t1.tv_usec;
	sec = t2.tv_sec - t1.tv_sec;

	//printf("\nerror: sec %ld usec %ld\n", sec, usec);
	
	if (usec < 0){
		sec--;
		usec += 1000000;
	}
	double result = (((double) sec) + ((double) usec)/1000000.0);
	//printf("error: %f\n", result);
	return result;
}

void printtime(struct rusage* rusage1, struct rusage* rusage2)
{
	double totaltime = 0;
	struct timeval t1 = rusage1->ru_utime;
	struct timeval t2 = rusage2->ru_utime;
	double t = timediff(t1, t2);
	totaltime += t;
	printf("\nUser time: %fs\n", t);

	t1 = rusage1->ru_stime;
	t2 = rusage2->ru_stime;
	t = timediff(t1, t2);
	totaltime += t;
	printf("System time: %fs\n", t);
	printf("Total time: %fs\n", totaltime);
	
}

typedef struct{
	pid_t pid;
	int start; //start of command
	int end; //end of command

} process_command;

int main(int argc, char* argv[])
{

	struct rusage overalltime1;
	struct rusage overalltime2;
	if (getrusage(RUSAGE_SELF, &overalltime1) < 0)
		fprintf(stderr, "error with getrusage\n");


	static int verbose = 0;
	static int profile = 0;
	int ret;
	int maxfiles = 3;
	int exitstatus = 0;
	int maxstatus = 0;
	int* fd = malloc(maxfiles*sizeof(int));
	int filecount = 0;
	process_command* commands = malloc(0);
	int processes = 0;

	int fileflags = 0;

	int startedtimer = 0;
	struct rusage rusage1;
	struct rusage rusage2;
	struct rusage rusagewait1;
	struct rusage rusagewait2;

	static struct option longopts[] = 
		{
			{"append", no_argument, NULL, 'a'},
			{"cloexec", no_argument, NULL, 'a'},
			{"creat", no_argument, NULL, 'a'},
			{"directory", no_argument, NULL, 'a'},
			{"dsync", no_argument, NULL, 'a'},
			{"excl", no_argument, NULL, 'a'},
			{"nofollow", no_argument, NULL, 'a'},
			{"nonblock", no_argument, NULL, 'a'},
			{"rsync", no_argument, NULL, 'a'},
			{"sync", no_argument, NULL, 'a'},
			{"trunc", no_argument, NULL, 'a'},
			{"rdonly", required_argument, NULL, 'b'},
			{"wronly", required_argument, NULL, 'b'},
			{"rdwr", required_argument, NULL, 'b'},
			{"pipe", no_argument, NULL, 'c'},
			{"command", required_argument, NULL, 'd'},
			{"wait", no_argument, NULL, 'e'},
			{"abort", no_argument, NULL, 'f'},
			{"catch", required_argument, NULL, 'g'},
			{"ignore", required_argument, NULL, 'h'},
			{"default", required_argument, NULL, 'i'},
			{"pause", no_argument, NULL, 'j'},
			{"close", required_argument, NULL, 'k'},
			{"verbose", no_argument, NULL, 'l'},
			{"profile", no_argument, &profile, 'm'},
			{0, 0, 0, 0}
		};

	int index = 0;
	while ((ret = getopt_long(argc, argv, "", longopts, &index)) != -1)
	{
		if (verbose)
		{
			printf("--%s ", longopts[index].name);
			if (optarg)
			  printf("%s ", optarg);
		}
		
		switch (ret)
		{
			case 0:
				//set flag only, do nothing else
				break;
			case 'a': //FILE FLAGS
				if (getrusage(RUSAGE_SELF, &rusage1) < 0)
					fprintf(stderr, "error with getrusage\n");
				startedtimer = 1;
				fileflags |= parse_flags(longopts[index].name);
				break;
			case 'b': //OPEN FILE
			{
				if (!startedtimer) //to time for all flags leading up to a file open, only retrieve a start time here if there were no other flags parsed
					if (getrusage(RUSAGE_SELF, &rusage1) < 0)
						fprintf(stderr, "error with getrusage\n");
				startedtimer = 0;

				if (optarg) //must take 1 arg
				{
					fileflags |= parse_flags(longopts[index].name);
					int f = open(optarg, fileflags, 0644);
					fileflags = 0;
					
					if (f < 0)
					{
						exitstatus = 1;
						fprintf(stderr, "error opening file %s\n", optarg); //just continue executing on error, but command wont work???
					}
					if (filecount == maxfiles)
					{
						maxfiles *= 2;
						fd = realloc(fd, maxfiles*sizeof(int));
					}
					fd[filecount] = f;
					filecount++;
					int extra = 0;
					while (optind < argc)
					{
						char* arg = argv[optind];
						if (strlen(argv[optind])>=3 && arg[0] == '-' && arg[1] == '-')
							break;
						extra = 1;
						if (verbose)
							printf("%s ", arg);
						optind++;
					}
					if (extra) //extra arguments
						fprintf(stderr, "error: extra argument to --%s option\n", longopts[index].name);
				}
				else
				{
					exitstatus = 1;
					fprintf(stderr, "error: --%s needs 1 arg: filename\n", longopts[index].name);
				}

				//Timing
				if (getrusage(RUSAGE_SELF, &rusage2) < 0)
					fprintf(stderr, "error with getrusage\n");
				if (profile)
					printtime(&rusage1, &rusage2);
				break;
			}
			case 'c': //CREATE PIPE
			{
				if (getrusage(RUSAGE_SELF, &rusage1) < 0)
					fprintf(stderr, "error with getrusage\n");

				if (filecount >= maxfiles - 1) //pipe requires 2 fds
				{
					maxfiles *= 2;
					fd = realloc(fd, maxfiles*sizeof(int));
				}
				if (pipe(&(fd[filecount])) == -1)
				{
					fd[filecount] = -1;
					fd[filecount+1] = -1;
					exitstatus = 1;
					fprintf(stderr, "error creating pipe");
				}
				filecount += 2;

				//Timing
				if (getrusage(RUSAGE_SELF, &rusage2) < 0)
					fprintf(stderr, "error with getrusage\n");
				if (profile)
					printtime(&rusage1, &rusage2);
				break;
			}
			case 'd': //EXECUTE COMMAND
			{
				if (getrusage(RUSAGE_SELF, &rusage1) < 0)
					fprintf(stderr, "error with getrusage\n");

				char* cmd;
				int input = atoi(optarg);
				int output;
				int error;

				int start_cmd_index;
				
				if (optind < argc - 2) //at least 3 more args left
				{
					output = atoi(argv[optind++]);
					error = atoi(argv[optind++]); //redirect error
					start_cmd_index = optind;
					cmd = argv[optind++]; //command string
					if (verbose)
						printf("%d %d %s ", output, error, cmd);
				}
				else
				{
					//error in number of options to --command
					fprintf(stderr, "error: --command takes 4 mandatory args: 3 files, 1 cmd\n");
					break;
				}

				int arglimit = 5;
				char** arglist = malloc(arglimit*sizeof(char*)); //initially allocate space for 5 args
				if (!arglist)
					fprintf(stderr, "error with dynamic allocation\n");
				arglist[0] = cmd;
				
				int i = 1;
				for (; i < arglimit; i++)
					arglist[i] = NULL;
				i = 1;
				while (optind < argc)
				{
					char* arg = argv[optind];
					if (strlen(argv[optind])>=3 && arg[0] == '-' && arg[1] == '-')
						break;
					//add to list of args
					if (i == arglimit-1) //must end in NULL, so realloc even before reaching arglimit
					{
						arglimit *= 2;
						arglist = realloc(arglist, arglimit*sizeof(char*));
						for (int j = i+1; j < arglimit; j++)
							arglist[j] = NULL;
					}
					arglist[i] = arg;
					if (verbose)
						printf("%s ", arg);
					optind++;
					i++;
				}

				//check if files are valid
				if (input >= filecount || output >= filecount || error >= filecount || fd[input] < 0 || fd[output] < 0 || fd[error] < 0)
				{
					fprintf(stderr, "cannot perform command %s on invalid file number(s)\n", cmd);
					exitstatus = 1;
					break;
				}


				int pid = fork();
				//keep track of command and args pertaining to current process, for printing later
				processes++;
				commands = realloc(commands, processes*sizeof(process_command));
				process_command* curr = &(commands[processes-1]);
				curr->pid = pid;
				curr->start = start_cmd_index;
				curr->end = optind;

				if (pid < 0)
				{
					fprintf (stderr, "failed to create process to run command\n");
					exit(1); //should program exit already??
				}
				else if (pid == 0) //child process
				{
					dup2(fd[input], 0); //redirect input
					dup2(fd[output], 1); //redirect output
					dup2(fd[error], 2); //redirect error

					for (i = 0; i < filecount; i++) //close unnecessary files, so pipes can receive EOF
						close(fd[i]);
					
					if (execvp(cmd, arglist) == -1)
					{
						//on error
						fprintf(stderr, "error executing command %s\n", cmd);
						exit(1);
					}
				}
				free(arglist);
				//optind = index - 1; //to deal with multiple args
				
				//Timing
				if (getrusage(RUSAGE_SELF, &rusage2) < 0)
					fprintf(stderr, "error with getrusage\n");
				if (profile)
					printtime(&rusage1, &rusage2);
				break;
			}
			case 'e': //WAIT
			{
				if (getrusage(RUSAGE_SELF, &rusage1) < 0)
					fprintf(stderr, "error with getrusage\n");
				if (getrusage(RUSAGE_CHILDREN, &rusagewait1) < 0)
					fprintf(stderr, "error with getrusage\n");

				int i;
				for (i = 0; i < filecount; i++) //close all files before wait
				{
					if (fd[i])
						close(fd[i]);
				}
				
				int status;
				
				pid_t pid;
				while ((pid = wait(&status)) != -1) //wait for all child processes
				{
					if (WIFEXITED(status))
					{
						int stat = WEXITSTATUS(status);
						if (stat > maxstatus)
							maxstatus = stat;
						printf("\n%d ", stat);
						int i;
						for (i = 0; i < processes; i++)
						{
							process_command* curr = &(commands[i]);
							if (curr->pid == pid)
							{
								for (i = curr->start; i < curr->end; i++)
									printf("%s ", argv[i]);
								break;
							}
						}
					}
				}

				//Timing
				if (getrusage(RUSAGE_SELF, &rusage2) < 0)
					fprintf(stderr, "error with getrusage\n");
				if (profile)
				{
					printf("\nPARENT TIME:");
					printtime(&rusage1, &rusage2);
				}
				if (getrusage(RUSAGE_CHILDREN, &rusagewait2) < 0)
					fprintf(stderr, "error with getrusage\n");
				if (profile)
				{
					printf("CHILDREN TIME:");
					printtime(&rusagewait1, &rusagewait2);
				}
				break;
			}
			case 'f': //SEG FAULT
				if (getrusage(RUSAGE_SELF, &rusage1) < 0)
					fprintf(stderr, "error with getrusage\n");

				raise(SIGSEGV);

				//Timing
				if (getrusage(RUSAGE_SELF, &rusage2) < 0)
					fprintf(stderr, "error with getrusage\n");
				if (profile)
					printtime(&rusage1, &rusage2);
				break;
			case 'g': //CATCH SIGNAL
				if (getrusage(RUSAGE_SELF, &rusage1) < 0)
					fprintf(stderr, "error with getrusage\n");

				if (optarg)
					signal(atoi(optarg), sig_handler);
				else
				{
					exitstatus = 1;
					fprintf(stderr, "error: --catch needs 1 arg: signal number\n");
				}

				if (getrusage(RUSAGE_SELF, &rusage2) < 0)
					fprintf(stderr, "error with getrusage\n");
				if (profile)
					printtime(&rusage1, &rusage2);
				break;
			case 'h': //IGNORE SIGNAL
				if (getrusage(RUSAGE_SELF, &rusage1) < 0)
					fprintf(stderr, "error with getrusage\n");

				if (optarg)
					signal(atoi(optarg), SIG_IGN);
				else
				{
					exitstatus = 1;
					fprintf(stderr, "error: --ignore needs 1 arg: signal number\n");
				}

				if (getrusage(RUSAGE_SELF, &rusage2) < 0)
					fprintf(stderr, "error with getrusage\n");
				if (profile)
					printtime(&rusage1, &rusage2);
				break;
			case 'i': //DEFAULT SIGNAL
				if (getrusage(RUSAGE_SELF, &rusage1) < 0)
					fprintf(stderr, "error with getrusage\n");

				if (optarg)
					signal(atoi(optarg), SIG_DFL);
				else
				{
					exitstatus = 1;
					fprintf(stderr, "error: --default needs 1 arg: signal number\n");
				}

				if (getrusage(RUSAGE_SELF, &rusage2) < 0)
					fprintf(stderr, "error with getrusage\n");
				if (profile)
					printtime(&rusage1, &rusage2);
				break;
			case 'j': //PAUSE
				if (getrusage(RUSAGE_SELF, &rusage1) < 0)
					fprintf(stderr, "error with getrusage\n");

				pause();
				
				if (getrusage(RUSAGE_SELF, &rusage2) < 0)
					fprintf(stderr, "error with getrusage\n");
				if (profile)
					printtime(&rusage1, &rusage2);
				break;
			case 'k': //CLOSE FILES
				if (getrusage(RUSAGE_SELF, &rusage1) < 0)
					fprintf(stderr, "error with getrusage\n");

				if (optarg)
				{
					int file = atoi(optarg);
					if (file >= filecount || (close(fd[file]) == -1))
					{
						fprintf(stderr, "error: cannot close invalid file %d\n", file);
						exitstatus = 1;
					}
					else
						fd[file] = -1;
				}
				else
				{
					exitstatus = 1;
					fprintf(stderr, "error: --close needs 1 arg: file number\n");
				}

				if (getrusage(RUSAGE_SELF, &rusage2) < 0)
					fprintf(stderr, "error with getrusage\n");
				if (profile)
					printtime(&rusage1, &rusage2);
				break;
			case 'l': //VERBOSE
				verbose = 1;
				break;
			case 'm':

				break;
			default:
				fprintf(stderr, "error: invalid option\n");
				exitstatus = 1;
		}
		//index++;
		if (verbose && ret != 'a')
			printf("\n");
	}
	free(fd);
	free(commands);

	if (maxstatus)
		exitstatus = maxstatus; //if maxstatus nonzero, exit with maxstatus; else exitstatus 0 if all options succeeded, 1 if any failed
	
	//Timing
	if (getrusage(RUSAGE_SELF, &overalltime2) < 0)
		fprintf(stderr, "error with getrusage\n");
	if (profile)
	{
		printf("OVERALL TIME FOR MAIN PROCESS, EXCLUDING CHILD COMMANDS:");
		printtime(&overalltime1, &overalltime2);
	}

	exit(exitstatus);
}	
