#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <dirent.h>

pid_t monitor_pid = -1;
int monitor_activ = 0;
int monitor_terminating = 0;
int pipefd[2]; //[0] citire; [1] scriere

struct sigaction sa;
struct dirent *entry;
struct dirent *entry_local;

//functii pentru semnale


void handle_signal(int sig)
{
  // folosit doar de monitor
  if (sig == SIGUSR1) {
    int fd = open("monitor_cmd.txt", O_RDONLY);
    if (fd<0) {
      perror("Monitor: nu pot deschide fișierul de comenzi");
      return;
    }

    char cmd[256];
    int n = read(fd, cmd, sizeof(cmd) - 1);
    if (n <= 0) {
      perror("Monitor: eroare la citirea comenzii din fișier");
      close(fd);
      return;
    }
    cmd[n] = '\0';

    // Elimină newline dacă există
    char *newline = strchr(cmd, '\n');
    if (newline) *newline = '\0';

    close(fd);

    int child=fork();
    if(child<0)
      {
	perror("Eroare creare proces copil pentru interpretare si executare comenzi");
	exit(1);
      }
    if(child==0)
      {
        if (strcmp(cmd, "list_hunts") == 0) {
	  execl("./treasure_manager", "./treasure_manager", "--list_hunts", NULL);
	  perror("Monitor: exec eșuat pentru list_hunts");
	  exit(1);
        } else if (strncmp(cmd, "--list_treasures ", 17) == 0) {
	  char *hunt_id = cmd + 17;
	  while( *hunt_id==' ') hunt_id++;
	  if(*hunt_id=='\0')
	    {
	      printf("Monitor: lipseste id-ul vanatorului pentru list_treasures\n");
	      exit(1);
	    }
	    
	  execl("./treasure_manager", "./treasure_manager", "--list_treasures", hunt_id, NULL);
	  perror("Monitor: exec eșuat pentru list_treasures");
	  exit(1);
	      
        } else if (strncmp(cmd, "--view_treasure ", 16) == 0){
	  char *args=cmd+16;
	  char *hunt=strtok(args," ");
	  char *treasure_id=strtok(NULL," ");
	  if(hunt && treasure_id)
	    {
	      execl("./treasure_manager","./treasure_manager","--view_treasure",hunt,treasure_id,NULL);
	      perror("Monitor: exec failed");
	      exit(1);
	    }
	  else
	    {
	      perror("Eroare: nu am primit hunt_id si treasure_id pentru --view_treasure");
	      exit(1);
	    }
	}
	else {
	  perror("Monitor: comandă necunoscută");
        }
      } else if (child>0)
      {
	//proces parinte(monitor): asteapta copilul
	waitpid(child, NULL, 0);
      }
  } else if (sig == SIGTERM) {
    printf("Monitorul se închide...\n");
    usleep(1000000);
    exit(0);
  }
}


void setup_signal_handlers()
{
  sa.sa_handler=handle_signal;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags=0;

  sigaction(SIGUSR1, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);
}

//functii pentru procesul monitor

void monitor_loop()
{
  setup_signal_handlers();//activeaza functiile care trateaza semnalele
  dup2(pipefd[1], STDOUT_FILENO);
  dup2(pipefd[1],STDERR_FILENO);
  close(pipefd[0]);
  close(pipefd[1]);
  
  while(1)
    {
      pause(); //procesul asteapta pasiv sa primeasca un semnal
    }

}

void start_monitor()
{
  if(monitor_activ)
    {
      perror("Eroare: procesul monitor este deja activ");
      return;
    }

  if(pipe(pipefd)<0)
    {
      perror("Eroare la crearea pipe-ului");
      exit(1);
    }
  
  monitor_pid=fork();
  if(monitor_pid < 0)
    {
      perror("Eroare la crearea procesului monitor");
      close(pipefd[0]);
      close(pipefd[1]);
      return;
    }
  if(monitor_pid == 0)
    {
      monitor_loop(); //copil
       exit(0);
      
    }
  monitor_activ=1;
  monitor_terminating=0;
  printf("Procesul monitor a inceput cu PID %d\n", monitor_pid);
}

void stop_monitor()
{
  if (!monitor_activ) {
        printf("Monitorul nu este activ.\n");
        return;
    }

    kill(monitor_pid, SIGTERM);
    monitor_activ = 0;
    monitor_terminating = 1;

    // așteptăm să se termine
    int status;
    waitpid(monitor_pid, &status, 0);

    if (WIFEXITED(status)) {
        printf("Monitorul s-a închis cu codul %d.\n", WEXITSTATUS(status));
    } else {
        printf("Monitorul s-a închis necontrolat.\n");
    }

    close(pipefd[0]);
    close(pipefd[1]);
    
    monitor_pid = -1;
    monitor_terminating = 0;
}


void send_command_to_monitor(const char *cmd) {
    if (!monitor_activ) {
        perror("Eroare: Monitorul nu este activ. Porniți-l cu --start_monitor");
        return;
    }

    int fd = open("monitor_cmd.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("Eroare la deschiderea fișierului de comandă");
        return;
    }
    dprintf(fd,"%s\n",cmd);
    close(fd);

    
    kill(monitor_pid, SIGUSR1);

    usleep(1000000);

    char buffer[2048];
    int n=read(pipefd[0], buffer, sizeof(buffer)-1);
    if(n>0)
      {
	buffer[n]='\0';
	printf("%s\n", buffer);
      }
    else
      printf("Monitorul nu a returnat nimic.\n");
}



////Functie pentru faza 3
void calculate_scores()
{
  DIR *dir;
  dir=opendir(".");
  if(!dir)
    {
      perror("Eroare deschidere director pentru functia calculate_scores");
      exit(-1);
    }

  while((entry_local=readdir(dir))!=NULL)
    {
      if(entry_local->d_type == DT_DIR && entry_local->d_name[0]!='.')
	{
	  int pipe_local[2];
	  if(pipe(pipe_local)==-1)
	    {
	      perror("Eroare la creare pipe local");
	      continue;
	    }
	  int pid=fork();
	  if(pid<0)
	    {
	      perror("Eroare fork calculate_scores");
	      continue;
	    }
	  if(pid==0)
	    {
	      // Copil: scrie în pipe
	      close(pipe_local[0]); // închide capătul de citire
	      dup2(pipe_local[1], STDOUT_FILENO); // redirecționează stdout
	      close(pipe_local[1]);

	      execl("./calculate_score", "./calculate_score", entry_local->d_name, NULL);
	      perror("Eroare la execl calculate_score");
	      exit(1);
	    }
	  else
	    {
	      // Părinte: citește din pipe
	      close(pipe_local[1]); // închide capătul de scriere

	      char buf[1024];
	      int n;
	      printf("Scoruri pentru %s:\n", entry_local->d_name);
	      while ((n = read(pipe_local[0], buf, sizeof(buf) - 1)) > 0) {
		buf[n] = '\0';
		printf("%s", buf);
	      }
	      close(pipe_local[0]);
	      waitpid(pid, NULL, 0); // așteaptă copilul curent
	
	    }
	}
    }

      closedir(dir);
     
}

int main()
{

  char command[256];


  while(1)
    {
      printf("> ");
      int len = read(STDIN_FILENO, command, sizeof(command) - 1);
      if (len <= 0) {
	perror("Eroare citire comanda");
	exit(1);
      }
      command[len]='\0';
      char *newline = strchr(command, '\n');
      if (newline) *newline = '\0';
      
      if (monitor_terminating) {
	printf("Monitorul este în curs de oprire. Așteptați finalizarea.\n");
	continue;
      }

      if (strcmp(command, "--start_monitor") == 0) {
	start_monitor();
        }
        else if (strcmp(command, "--stop_monitor") == 0) {
            stop_monitor();
        }
        else if (strcmp(command, "--list_hunts") == 0) {
	  send_command_to_monitor("list_hunts");
	  //char *args[] = {"./treasure_manager", "--list_hunts", NULL};
	  //run_manager_command(args, "Listă de vânători");
        }
        else if (strncmp(command, "--list_treasures ", 17) == 0) {
	  send_command_to_monitor(command);
        }
        else if (strncmp(command, "--view_treasure ", 16) == 0) {
	  send_command_to_monitor(command);
        }
	else if(strcmp(command, "--calculate_score")==0)
	  {
	    calculate_scores();
	  }
        else if (strcmp(command, "--exit") == 0) {
            if (monitor_activ) {
                perror("Eroare: Monitorul este încă activ. Opriți-l cu --stop_monitor înainte de a ieși.\n");
                continue;
            }
            break;
        }
        else {
            printf("Comandă necunoscută: %s\n", command);
        }
    }
  return 0;
}
