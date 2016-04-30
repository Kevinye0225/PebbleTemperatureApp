/* 
This code primarily comes from 
http://www.prasannatech.net/2008/07/socket-programming-tutorial.html
and
http://www.binarii.com/files/papers/c_sockets.txt
 */
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <termios.h>
#include <pthread.h>

float temperature = 0;
int is_celsius = 0;
int is_standby = 0;
int is_api = 0;
float current_tempF = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
int end_con = 0;
int arduino_close = 1;

char api_temp[255];
float philly_temp = 0.0;






void* start_server(void* p);
void* get_temp(void* p);
void* stop(void* p);


void* start_server(void* p)
{
      char buff[255];
      int bytes_read = 0;
      int count = 0;
      char mem[255];
      int found = 0;
      float tempe = 0;
      int PORT_NUMBER = *((int*) p);
      int pointer = 0;
      int isListFull = 0;
      int notFirstElement = 0;
      int request_counter = 6;
      int api_counter = 0;
      char arduino_error[] = "{\n\"arduino\": \"arduino close\"}";

      float temperatureList[3600];
      float max_temp = -100;
      float min_temp = 1000;
      float avg_temp = -100;
      float max_tempF = -100;
      float min_tempF = 1000;
      float avg_tempF = -100;
      float prev_temp = -100;


      // structs to represent the server and client
      struct sockaddr_in server_addr,client_addr;    
      
      int sock; // socket descriptor

      printf("Start\n");

      // 1. socket: creates a socket descriptor that you later use to make other system calls
      if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        	perror("Socket");
        	exit(1);
      }
      int temp;
      if (setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&temp,sizeof(int)) == -1) {
	perror("Setsockopt");
	exit(1);
      }

      // configure the server
      server_addr.sin_port = htons(PORT_NUMBER); // specify port number
      server_addr.sin_family = AF_INET;         
      server_addr.sin_addr.s_addr = INADDR_ANY; 
      bzero(&(server_addr.sin_zero),8); 
      
      // 2. bind: use the socket and associate it with the port number
      if (bind(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
	perror("Unable to bind");
	exit(1);
      }

      // 3. listen: indicates that we want to listn to the port to which we bound; second arg is number of allowed connections
      if (listen(sock, 5) == -1) {
	perror("Listen");
	exit(1);
      }
          
      // once you get here, the server is set up and about to start listening
      printf("\nServer configured to listen on port %d\n", PORT_NUMBER);
      fflush(stdout);

      while(1){

          pthread_mutex_lock(&lock);
          if (end_con == 1){
            pthread_mutex_unlock(&lock);
            break;
          }
          pthread_mutex_unlock(&lock);
         
          // 4. accept: wait here until we get a connection on that port
          int sin_size = sizeof(struct sockaddr_in);
          int fd = accept(sock, (struct sockaddr *)&client_addr,(socklen_t *)&sin_size);
          if (fd != -1) {
            printf("Server got a connection from (%s, %d)\n", inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
            
            // buffer to read data into
            char request[1024];
            is_standby = 0;
            
            // 5. recv: read incoming message into buffer
            int bytes_received = recv(fd,request,1024,0);
            // null-terminate the string
            request[bytes_received] = '\0';

            printf("Here comes the message:\n");
            printf("%s\n", request);

            if (request[5] == 'f'){
              is_celsius = 1;
              printf("Now fahrenheit\n");
            }else{
              is_celsius = 0;
            }

            if (request[5] == 's'){
              is_standby = 1;
              printf("Now standby\n");
            } else {
              is_standby = 0;
            }

            //check if displaying api
            if (request[5] == 't'){
              is_api = 1;
              while (request[request_counter] != ' '){
                api_temp[api_counter] = request[request_counter];
                request_counter++;
                api_counter++;
              }
              api_temp[api_counter] = '\0';
              // printf("%s\n", api_temp);
              sscanf(api_temp, "%f", &philly_temp);
              printf("%f\n", philly_temp);
              api_temp[0] = '\0';
              request_counter = 6;
              // check if it is integer
              api_counter = 0;
              // printf("%s\n", api_temp);


              printf("Now weather api\n");
            } else {
              is_api = 0;
            }

            char *reply = "{\n\"name\": \"";
            char msg[1024];

            strcpy(msg, reply);

            sprintf(mem, "%0.4f", temperature);

            strcat(msg, mem);
            strcat(msg, "\", \n\"avg\": \"");


            // Calculating Average
            if (isListFull == 1){
              prev_temp = temperatureList[pointer];
            }

            //store the temperature
            temperatureList[pointer] = temperature;

            if (avg_temp == -100) {
              avg_temp = temperature;
            } else if (isListFull == 0) {
              avg_temp += (temperature-avg_temp)/(pointer+1);
            } else {
              avg_temp = (avg_temp * 3600 - prev_temp + temperature) / 3600.0;
            }

            if (pointer == 3599){
              isListFull = 1;          
            }
            pointer = (pointer+1)%3600;



            //Max and min

            int i = 0;
            while (i < sizeof(temperatureList)){
              if (temperatureList[i] > max_temp){
                max_temp = temperatureList[i];
              }
              if (temperatureList[i] < min_temp){
                min_temp = temperatureList[i];
              }
              i++;
            }
            
            
            avg_tempF = (avg_temp*9/5)+32;
            max_tempF = (max_temp*9/5)+32;
            min_tempF = (min_temp*9/5)+32;


            mem[0] = '\0';
            sprintf(mem, "%0.4f", avg_temp);
            strcat(msg, mem);
            strcat(msg, "\", \n\"min\": \"");

            mem[0] = '\0';
            sprintf(mem, "%0.4f", min_temp);
            strcat(msg, mem);
            strcat(msg, "\", \n\"max\": \"");

            mem[0] = '\0';
            sprintf(mem, "%0.4f", max_temp);
            strcat(msg, mem);

// Send fahrenheit 
            strcat(msg, "\", \n\"currentF\": \"");
            mem[0] = '\0';
            sprintf(mem, "%0.4f", current_tempF);
            strcat(msg, mem);

            strcat(msg, "\", \n\"avgF\": \"");
            mem[0] = '\0';
            sprintf(mem, "%0.4f", avg_tempF);
            strcat(msg, mem);

            strcat(msg, "\", \n\"minF\": \"");
            mem[0] = '\0';
            sprintf(mem, "%0.4f", min_tempF);
            strcat(msg, mem);

            strcat(msg, "\", \n\"maxF\": \"");
            mem[0] = '\0';
            sprintf(mem, "%0.4f", max_tempF);
            strcat(msg, mem);

            strcat(msg, "\"\n}\n");
            if (arduino_close == 1){
              send(fd, arduino_error, strlen(arduino_error), 0);
            } else {
              send(fd, msg, strlen(msg), 0);
            }

            // 6. send: send the message over the socket
            // note that the second argument is a char*, and the third is the number of chars
            // send(fd, reply, strlen(reply), 0);
            //printf("Server sent message: %s\n", reply);

            // 7. close: close the socket connection
            close(fd);
          }
        }

        close(sock);
        printf("Server closed connection\n");
        // pthread_mutex_unlock(&lock);
        
      return 0;
} 

void* get_temp(void* p){
    int count = 0;
    char mem[255];
    int found = 0;
    float tempe = 0;
    char buff[255];
    char fahrenheit[255];
    int bytes_read = 0;
    char standBy[3];
    char api[255];
    int arduino;



    while (arduino_close == 1){
      pthread_mutex_lock(&lock);
      if (end_con == 1){
        pthread_mutex_unlock(&lock);
        break;
      }
      pthread_mutex_unlock(&lock);
      arduino = open("/dev/cu.usbmodem1411", O_RDWR);
      // int arduino = open("/dev/ttyUSB10", O_RDWR);
      if  (arduino == -1){
        printf("Fail to open arduino\n");
        continue;
      } 


      struct termios options;
      // struct to hold options
      tcgetattr(arduino, &options);
      // associate with this fd
      cfsetispeed(&options, 9600); // set input baud rate
      cfsetospeed(&options, 9600); // set output baud rate
      tcsetattr(arduino, TCSANOW, &options); // set options
      printf("opening arduino\n");
      arduino_close = 0;



    while(1){
        if (end_con == 1){
          close(arduino);
          break;
        }
          
        bytes_read = read(arduino,buff,1);
        if(bytes_read !=0) {

          if ((buff[0] >= '0' && buff[0] <= '9') || buff[0] == '.'){
            found = 1;
            mem[count] = buff[0];
            count++;
          } else {
            found = 0;
          }

          if (count != 0 && found == 0){
            mem[count+1] = '\0';
            sscanf(mem, "%f", &temperature);
            current_tempF = (temperature*9/5)+32;
            sprintf(fahrenheit, "%0.4f", current_tempF);
            count = 0;
            int len = strlen(fahrenheit);
            fahrenheit[len] = '\n';
            fahrenheit[len+1] = '\0';
            standBy[0] = '-';
            standBy[1] = '\n';
            standBy[2] = '\0';
            // printf("%s\n", fahrenheit);
            // printf("%s\n", mem);
          }
          if (is_standby == 1){
              int bytes_written = write(arduino, standBy, strlen(standBy));
              standBy[0] = '\0';
          } else {
            if (is_api == 1){
              // int bytes_written = write(arduino, fahrenheit, strlen(fahrenheit));
              // printf("%s\n", api_temp);
              sprintf(api, "%0.2f", philly_temp);
              int api_len = strlen(api);
              api[api_len] = 'c';
              api[api_len+1] = '\n';
              api[api_len+2] = '\0';
              // printf("%d\n", strlen(api));


              int bytes_written = write(arduino, api, strlen(api));


              // printf("%s\n", api);
              api[0] = '\0';


            } else {
              if (is_celsius == 1){
                  int  bytes_written = write(arduino, fahrenheit, strlen(fahrenheit));
                  // printf("%s\n", fahrenheit);
                  fahrenheit[0] = '\0';
              }
            }
            
          }
          

        }else{
          // Disconnect error
          arduino_close = 1;
          close(arduino);
          printf("Disconnected\n");
          break;
        }
      }
    }


      // close(arduino);
      printf("arduino Disconnected\n");
      return NULL;

}

void* stop(void* p){
  while (1){
    char input[100] = {'\0'};
    printf("Please enter q and make a new http request to quit: \n");
    scanf("%s", input);

    if (input[0] == 'q'){
      pthread_mutex_lock(&lock);
      end_con = 1;
      pthread_mutex_unlock(&lock);
      return NULL;
    }

    // pthread_mutex_unlock(&lock);

  }
  // return NULL;
}


int main(int argc, char *argv[])
{
  // check the number of arguments
  pthread_t t1, t2, t3;

  if (argc != 2)
    {
      printf("\nUsage: server [port_number]\n");
      exit(0);
    }

  int PORT_NUMBER = atoi(argv[1]);

  if (PORT_NUMBER <= 1024) {
    printf("\nPlease specify a port number greater than 1024\n");
    exit(-1);
  }

  pthread_create(&t3, NULL, &stop, NULL);

  pthread_create(&t2, NULL, &get_temp, NULL);
  pthread_create(&t1, NULL, &start_server, &PORT_NUMBER);

  pthread_join(t1, NULL);
  pthread_join(t2, NULL);
  pthread_join(t3, NULL);

  printf("Done\n");

  return 0;
  // start_server(((int*)&PORT_NUMBER);
}

