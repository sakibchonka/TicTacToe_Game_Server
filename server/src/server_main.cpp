#include <ctime>
#include <iostream>
#include <string>
#include <asio.hpp>
#include <thread>

using namespace std;
using asio::ip::tcp;

int player_count = 0;
pthread_mutex_t mutexcount;
tcp::socket *ptr_socket;

void error(const char *msg)
{
    perror(msg);
    pthread_exit(NULL);
}

void write_client_int(int cli_sockfd, int msg)
{
    int n = write(cli_sockfd, &msg, sizeof(int));
    if (n < 0)
        error("ERROR writing int to client socket");
}


void write_client_msg(int cli_sockfd, const char * msg)
{

    int n = write(cli_sockfd, msg, strlen(msg));
    if (n < 0)
        error("ERROR writing msg to client socket");
}


void write_clients_msg(int *cli_sockfd, const char * msg)
{
    write_client_msg(cli_sockfd[0], msg);
    write_client_msg(cli_sockfd[1], msg);
}


void write_clients_int(int * cli_sockfd, int msg)
{
    write_client_int(cli_sockfd[0], msg);
    write_client_int(cli_sockfd[1], msg);
}

int recv_int(int cli_sockfd)
{
    int msg = 0;
    int n = read(cli_sockfd, &msg, sizeof(int));
    
    if (n < 0 || n != sizeof(int))  return -1;

    printf("[DEBUG] Received int: %d\n", msg);
    
    return msg;
}


int get_player_move(int cli_sockfd)
{
    #ifdef DEBUG
    printf("[DEBUG] Getting player move...\n");
    #endif
    
    write_client_msg(cli_sockfd, "TRN");

    return recv_int(cli_sockfd);
}


int check_move(char board[][3], int move, int player_id)
{
    if ((move == 9) || (board[move/3][move%3] == ' ')) { 
        
        #ifdef DEBUG
        printf("[DEBUG] Player %d's move was valid.\n", player_id);
        #endif
        
        return 1;
   }
   else {       
       #ifdef DEBUG
       printf("[DEBUG] Player %d's move was invalid.\n", player_id);
       #endif
    
       return 0;
   }
}


void update_board(char board[][3], int move, int player_id)
{
    board[move/3][move%3] = player_id ? 'X' : 'O';
    
    #ifdef DEBUG
    printf("[DEBUG] Board updated.\n");
    #endif
}


void draw_board(char board[][3])
{
    printf(" %c | %c | %c \n", board[0][0], board[0][1], board[0][2]);
    printf("-----------\n");
    printf(" %c | %c | %c \n", board[1][0], board[1][1], board[1][2]);
    printf("-----------\n");
    printf(" %c | %c | %c \n", board[2][0], board[2][1], board[2][2]);
}


void send_update(int * cli_sockfd, int move, int player_id)
{
    #ifdef DEBUG
    printf("[DEBUG] Sending update...\n");
    #endif
    
    write_clients_msg(cli_sockfd, "UPD");

    write_clients_int(cli_sockfd, player_id);
    
    write_clients_int(cli_sockfd, move);
    
    #ifdef DEBUG
    printf("[DEBUG] Update sent.\n");
    #endif
}

void send_player_count(int cli_sockfd)
{
    write_client_msg(cli_sockfd, "CNT");
    write_client_int(cli_sockfd, player_count);

    #ifdef DEBUG
    printf("[DEBUG] Player Count Sent.\n");
    #endif
}

int check_board(char board[][3], int last_move)
{
    #ifdef DEBUG
    printf("[DEBUG] Checking for a winner...\n");
    #endif
   
    int row = last_move/3;
    int col = last_move%3;

    if ( board[row][0] == board[row][1] && board[row][1] == board[row][2] ) { 
        #ifdef DEBUG
        printf("[DEBUG] Win by row %d.\n", row);
        #endif 
        return 1;
    }
    else if ( board[0][col] == board[1][col] && board[1][col] == board[2][col] ) { 
        #ifdef DEBUG
        printf("[DEBUG] Win by column %d.\n", col);
        #endif 
        return 1;
    }
    else if (!(last_move % 2)) { if ( (last_move == 0 || last_move == 4 || last_move == 8) && (board[1][1] == board[0][0] && board[1][1] == board[2][2]) ) {  
            #ifdef DEBUG
            printf("[DEBUG] Win by backslash diagonal.\n");
            #endif 
            return 1;
        }
        if ( (last_move == 2 || last_move == 4 || last_move == 6) && (board[1][1] == board[0][2] && board[1][1] == board[2][0]) ) { 
            #ifdef DEBUG
            printf("[DEBUG] Win by frontslash diagonal.\n");
            #endif 
            return 1;
        }
    }

    #ifdef DEBUG
    printf("[DEBUG] No winner, yet.\n");
    #endif
    
    return 0;
}


//void *run_game(void *thread_data, tcp::socket *ptr_socket)
void *run_game(void *thread_data, tcp::socket *ptr_socket)
{
    int *cli_sockfd = (int*)thread_data;

    char board[3][3] = { {' ', ' ', ' '},  
                         {' ', ' ', ' '}, 
                         {' ', ' ', ' '} };

    printf("Game on!\n");
    
    write_clients_msg(cli_sockfd, "SRT");

    #ifdef DEBUG
    printf("[DEBUG] Sent start message.\n");
    #endif

    draw_board(board);
    
    int prev_player_turn = 1;
    int player_turn = 0;
    int game_over = 0;
    int turn_count = 0;
    while(!game_over) {
        
        if (prev_player_turn != player_turn)
            write_client_msg(cli_sockfd[(player_turn + 1) % 2], "WAT");

        int valid = 0;
        int move = 0;
        while(!valid) {             
            move = get_player_move(cli_sockfd[player_turn]);
            if (move == -1) break; 
            printf("Player %d played position %d\n", player_turn, move);
                
            valid = check_move(board, move, player_turn);
            if (!valid) { 
                printf("Move was invalid. Let's try this again...\n");
                write_client_msg(cli_sockfd[player_turn], "INV");
            }
        }

	    if (move == -1) { 
            printf("Player disconnected.\n");
            break;
        }
        else if (move == 9) {
            prev_player_turn = player_turn;
            send_player_count(cli_sockfd[player_turn]);
        }
        else {

            update_board(board, move, player_turn);
            send_update( cli_sockfd, move, player_turn );
                
         
            draw_board(board);

            game_over = check_board(board, move);
            
            if (game_over == 1) {
                write_client_msg(cli_sockfd[player_turn], "WIN");
                write_client_msg(cli_sockfd[(player_turn + 1) % 2], "LSE");
                printf("Player %d won.\n", player_turn);
            }
            else if (turn_count == 8) {                
                printf("Draw.\n");
                write_clients_msg(cli_sockfd, "DRW");
                game_over = 1;
            }

            prev_player_turn = player_turn;
            player_turn = (player_turn + 1) % 2;
            turn_count++;
        }
    }

    printf("Game over.\n");

	close(cli_sockfd[0]);
    close(cli_sockfd[1]);

    pthread_mutex_lock(&mutexcount);
    player_count--;
    printf("Number of players is now %d.\n", player_count);
    player_count--;
    printf("Number of players is now %d.\n", player_count);
    pthread_mutex_unlock(&mutexcount);
    
    delete []cli_sockfd;
    // delete []ptr_socket;
    ptr_socket->close();
    delete ptr_socket;
    
    return 0;
}


int main(int argc, char *argv[])
{
  try
  {
    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }
  
    char board[3][3] = { {' ', ' ', ' '},
                         {' ', ' ', ' '},
                         {' ', ' ', ' '} };
  /*
    cout << endl << "Wait for Server to load..!!" << endl;
    cout << endl << "Will load in 10 Sec " << endl << "Loading "; 
    for(int i = 0; i < 10; i++) {
	    cout << ". ";
	    cout.flush();
	    sleep(1);
    }



float progress = 0.0;
while (progress <= 1.0) {
    int barWidth = 70;

    std::cout << "[";
    int pos = barWidth * progress;
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << int(progress * 100.0) << " %\r";
    std::cout.flush();
    sleep(1);

    progress += 0.16; // for demonstration only
}
std::cout << std::endl;

*/

    cout << endl << endl;
    cout << "WELCOME TO THE TIC-TAC-TOE SERVER GAME" << endl;
    cout << "--------------------------------------" << endl;
    draw_board(board);
    cout << endl << "Wait for Client to connect..!!" << endl;

    asio::io_context io_context;
    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), atoi(argv[1])));

    pthread_mutex_init(&mutexcount, NULL); 

    while(1)
    {
        if (player_count <= 2){
            int *cli_sockfd = (int*)malloc(2*sizeof(int));
            memset(cli_sockfd, 0, 2*sizeof(int));
            
	    
	    int num_conn = 0;
            while(num_conn < 2 && player_count <= 2){
          
            tcp::socket *ptr_socket = new tcp::socket(io_context);
            #ifdef DEBUG
            printf("[DEBUG] Listening for clients...\n");
            #endif

                acceptor.accept(*ptr_socket);
                cli_sockfd[num_conn] = ptr_socket->native_handle();

		if (cli_sockfd[num_conn] < 0)
                   error("ERROR accepting a connection from a client.");

                #ifdef DEBUG
                printf("[DEBUG] Accepted connection from client %d\n", num_conn);
                #endif

                write(cli_sockfd[num_conn], &num_conn, sizeof(int));

                #ifdef DEBUG
                printf("[DEBUG] Sent client %d it's ID.\n", num_conn); 
                #endif 
                
                pthread_mutex_lock(&mutexcount);
                player_count++;
                printf("Number of players is now %d.\n", player_count);
                pthread_mutex_unlock(&mutexcount);

                if (num_conn == 0) {
                    write_client_msg(cli_sockfd[0],"HLD");
                    
                    #ifdef DEBUG
                    printf("[DEBUG] Told client 0 to hold.\n");
                    #endif 
                }

                num_conn++;

// delete []ptr_socket;
            }
        
            #ifdef DEBUG
            printf("[DEBUG] Starting new game thread...\n");
            #endif
        
//	    std::thread thread1(run_game, (void*)cli_sockfd, ptr_socket);
	    std::thread thread1(run_game, (void*)cli_sockfd, ptr_socket);
	    thread1.detach();

	    //delete ptr_socket;

	    printf("[DEBUG] New game thread started.\n");
	}	
    }
  }
  catch (std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }

  pthread_mutex_destroy(&mutexcount);
  pthread_exit(NULL);
  return 0;
}
