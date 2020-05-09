// Ozan ERCAN 2035921

#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "logging.c"
#include <sys/poll.h>
#include <string.h>
#include <ctype.h>
#include <wait.h>


#define PIPE(fd) socketpair(AF_UNIX, SOCK_STREAM, PF_UNIX, fd)

//get executable line
char *getline2(void) {
    char *line = malloc(100), *linep = line;
    size_t lenmax = 100, len = lenmax;
    int c;
    if (line == NULL)
        return NULL;
    for (;;) {
        c = fgetc(stdin);
        if (c == EOF)
            break;

        if (--len == 0) {
            len = lenmax;
            char *linen = realloc(linep, lenmax *= 2);

            if (linen == NULL) {
                free(linep);
                return NULL;
            }
            line = linen + (line - linep);
            linep = linen;
        }

        if ((*line++ = c) == '\n')
            break;
    }
    *line = '\0';
    return linep;
}

//helper
static char *myStrDup(char *str) {
    char *other = malloc(strlen(str) + 1);
    if (other != NULL)
        strcpy(other, str);
    return other;
}


//********************************Check if there are active pfd ********************************************************
int pollcheck(int num_of_bidder, struct pollfd *pfd) {
    int m = 0;
    while (num_of_bidder > m) {
        if (pfd[m].fd < 0)
            m++;
        else return 1;
    }
    return 0;
}


int main() {
//*********************getting first three integer**********************************************************************

    int starting_bid, min_inc;
    int num_of_bidder = 0, j = 0;
    char option[64], line[256];
    int numbers[3];

    if (fgets(line, sizeof(line), stdin)) {
        if (1 == sscanf(line, "%[^\n]%*c", option)) {//[^\n] isn't newline chars
            //printf("%s\n", option);
        }
    }
    char *str2 = option, *p = str2;
    while (*p) { // While there are more characters to process...
        if (isdigit(*p) || ((*p == '-' || *p == '+') && isdigit(*(p + 1)))) {
            // Found a number
            long val = strtol(p, &p, 10); // Read number
            numbers[j] = val;
            j++;
        } else {
            // Otherwise, move on to the next character.
            p++;
        }
    }
    starting_bid = numbers[0];
    min_inc = numbers[1];
    num_of_bidder = numbers[2];

    pid_t pid_array[num_of_bidder];


//******************************creating Socket Pipes ******************************************************************

    int fdArray[num_of_bidder][2];
    int pid;
    int i = 0;
    while (num_of_bidder > i) {
        PIPE(fdArray[i]);
        i++;
    }

    int x[num_of_bidder];
    i = 0;
    while (num_of_bidder > i) {
        x[i] = fdArray[i][0];
        i++;
    }

//**********************************Getting arguments from console and create arguments array **************************

    i = 0;
    char *argv2[num_of_bidder][200];

    while (i < num_of_bidder) {

        int argc = 0;
        char *str = strtok(getline2(), " ");
        while (str != NULL) {
            argv2[i][argc++] = myStrDup(str);
            str = strtok(NULL, " ");
        }
        argv2[i][argc] = NULL;
        for (int c = 1; c < argc - 1; c++)
            argv2[i][c] = argv2[i][c + 1];

        argv2[i][argc - 1] = NULL;
        i++;
    }
//****************************fork, dup and exec part ******************************************************************

    for (i = 0; i < num_of_bidder; i++) // loop will run n times (n=5)
    {
        pid_array[i] = fork();
        if (pid_array[i] == 0) {
            dup2(x[i], 0);
            dup2(x[i], 1);
            execvp(argv2[i][0], argv2[i]);
        }
       // else break;
    }


//**************************parent**************************************************************************************
    //if (pid > 0) {

        int client_id;
        int current_bid = 0;
        int winner_bidder = -1;
        int remaining_bidders = num_of_bidder;
        int n, r;
        int status[num_of_bidder];

        oi oi_data;
        ii ii_data;
        cm cm2;
        sm sm3;

        struct pollfd pfd[num_of_bidder]; // = {{ fdArray[0][1], POLLIN, 0}, { fdArray[1][1], POLLIN, 0}} ;

        i = 0;
        while (num_of_bidder > i) {
            struct pollfd a = {fdArray[i][1], POLLIN, 0};
            pfd[i] = a;
            i++;
        }

        n = num_of_bidder;

        while (pollcheck(num_of_bidder, pfd)) {
            poll(pfd, n, 0);
            for (i = 0; i < n; i++)
                if (pfd[i].revents && POLLIN) {
                    r = read(pfd[i].fd, &cm2, sizeof(cm));
                    if (r == 0 && cm2.message_id == SERVER_AUCTION_FINISHED) { /* EOF */
                        pfd[i].fd = -1;   /* poll() ignores pollfd item if fd is negative */
                    } else {

                        client_id = i;
                        ii_data.type = cm2.message_id;
                        ii_data.pid = pid_array[i];
                        ii_data.info = cm2.params;

                        if (cm2.message_id == SERVER_AUCTION_FINISHED) {
                            --remaining_bidders;
                            pfd[i].fd = -1;

                        }
                        if ( remaining_bidders==0){
                            print_input(&ii_data, client_id);
                            winner_bidder= i;
                            break;
                        }


                        print_input(&ii_data, client_id);


                        //client_connecte cevabÄ± burada , use start_info
                        if (cm2.message_id == 1) {
                            sm3.message_id = 1; //non_zero and unique message id
                            sm3.params.start_info.client_id = client_id;
                            sm3.params.start_info.current_bid = current_bid;
                            sm3.params.start_info.minimum_increment = min_inc;
                            sm3.params.start_info.starting_bid = starting_bid;
                        }

                        //client_bid cevabÄ± burada , use result_info
                        if (cm2.message_id == 2) {
                            sm3.message_id = 2;
                            sm3.params.result_info.current_bid = current_bid;


                            //BID_LOWER_THAN_STARTING_BID 1
                            if (starting_bid > cm2.params.bid) {
                                sm3.params.result_info.result = 1;
                            }
                            //BID_LOWER_THAN_CURRENT 2
                            if (current_bid > cm2.params.bid) {
                                sm3.params.result_info.result = 2;
                            }
                            //BID_INCREMENT_LOWER_THAN_MINIMUM 3
                            if (min_inc > cm2.params.bid - current_bid) {
                                sm3.params.result_info.result = 3;
                            }
                                //BID_ACCEPTED 0
                            else {
                                sm3.params.result_info.result = 0;
                                current_bid = cm2.params.bid;
                                //winner_bidder = client_id;
                            }

                        }

                        //client_finished cevabÄ±  , use winner_info
                        //This message is sent to all the bidders after they stop bidding and
                        //the winner is determined. It uses winner_info field from the parameters. The winner_id should
                        //contain the id of the winning bidder and the winning_bid should contain the winning bid amount.


                        oi_data.type = sm3.message_id;
                        oi_data.pid = pid_array[i];
                        oi_data.info = sm3.params;

                        print_output(&oi_data, client_id);



                        if (cm2.message_id != SERVER_AUCTION_FINISHED){
                            write(pfd[i].fd, &sm3, sizeof(sm));
                        }



                    }
                }
        }



        for( i=0; i<num_of_bidder ; i++){
            sm3.message_id = 3;
            sm3.params.winner_info.winner_id = winner_bidder;
            sm3.params.winner_info.winning_bid = current_bid;

            oi_data.type = sm3.message_id;
            oi_data.pid = pid_array[i];
            oi_data.info = sm3.params;

            //pfd[i].fd =1;

            write(pfd[i].fd, &sm3, sizeof(sm));
        }


        print_server_finished(winner_bidder,current_bid);


    for( i=0; i<num_of_bidder ; i++){
        sm3.message_id = 3;
        sm3.params.winner_info.winner_id = winner_bidder;
        sm3.params.winner_info.winning_bid = current_bid;

        oi_data.type = sm3.message_id;
        oi_data.pid = pid_array[i];;
        oi_data.info = sm3.params;

        pfd[i].fd =1;
        print_output(&oi_data, i);
    }

    for(i=0; i<num_of_bidder; i++){
        if(pid_array[i]>0){
            kill(pid_array[i], SIGKILL);
            status[i]=waitpid(pid_array[i], NULL, 0);
            close(fdArray[i][1]);
            pid_array[i] = 0;
        }
    }
    for( i=0; i<num_of_bidder ; i++){
        print_client_finished( i, 0, 1);

    }
    return 0;
}
