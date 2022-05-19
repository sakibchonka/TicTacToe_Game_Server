// run using cmd: g++ -I "../asio-1.22.1/include/" client_v2.cpp -lpthread"
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <iostream>
#include <asio.hpp>

using namespace std;
using asio::ip::tcp;

void error(const char *msg)
{
    perror(msg);
    printf("Either the server shut down or the other player disconnected.\nGame over.\n");
    
    exit(0);
}

int recv_int(int sockfd)
{
    int msg = 0;
    int n = read(sockfd, &msg, sizeof(int));
    
    // error checking
    if (n < 0 || n != sizeof(int)) 
        error("ERROR reading int from server socket");
    
    printf("[DEBUG] Received int: %d\n", msg);
    
    return msg;
}

void recv_msg(int sockfd, char * msg)
{
    memset(msg, 0, 4);
    int n = read(sockfd, msg, 3);
    
    if (n < 0 || n != 3)
     error("ERROR reading message from server socket.");

    printf("[DEBUG] Received message: %s\n", msg);
}

void draw_board(char board[3][3])
{
    printf(" %c | %c | %c \n", board[0][0], board[0][1], board[0][2]);
    printf("-----------\n");
    printf(" %c | %c | %c \n", board[1][0], board[1][1], board[1][2]);
    printf("-----------\n");
    printf(" %c | %c | %c \n", board[2][0], board[2][1], board[2][2]);
}

void write_server_int(int sockfd, int msg)
{
    int n = write(sockfd, &msg, sizeof(int));
    if (n < 0)
        error("ERROR writing int to server socket");
    
    printf("[DEBUG] Wrote int to server: %d\n", msg);
}

void take_turn(int sockfd)
{
    char buffer[10];
    
    while (1) { 
        printf("Enter 0-8 to make a move, or 9 for number of active players: ");
	    fgets(buffer, 10, stdin);
	    int move = buffer[0] - '0';
        if (move <= 9 && move >= 0){
            printf("\n");
            write_server_int(sockfd, move);   
            break;
        } 
        else
            printf("\nInvalid input. Try again.\n");
    }
}

void get_update(int sockfd, char board[][3])
{
    
    int player_id = recv_int(sockfd);
    int move = recv_int(sockfd);
    board[move/3][move%3] = player_id ? 'X' : 'O';    
}

int main(int argc, char *argv[])
{
    try
    {
        // validate no of arguments
        if (argc < 3)
        {
            fprintf(stderr,"usage %s hostname port\n", argv[0]);
            exit(0);
        }

        // All IO happens inside a context
        asio::io_context io_context;

        // Converting hostname port to endpoint address(es)
        tcp::resolver resolver(io_context); 
        tcp::resolver::results_type endpoints = resolver.resolve(argv[1], argv[2]);     

        // create a socket
        tcp::socket socket(io_context);

        // connect to server
        asio::connect(socket, endpoints);

        int sockfd = socket.native_handle(); // get the underlying socket descriptor
        
        int id = recv_int(sockfd);

        #ifdef DEBUG
        printf("[DEBUG] Client ID: %d\n", id);
        #endif 

        char msg[4];
        char board[3][3] = { {' ', ' ', ' '}, 
                            {' ', ' ', ' '}, 
                            {' ', ' ', ' '} };
        
        printf("Tic-Tac-Toe\n------------\n");

        do {
            recv_msg(sockfd, msg);
            if (!strcmp(msg, "HLD"))
                printf("Waiting for a second player...\n");
        } while ( strcmp(msg, "SRT") );

        /* The game has begun. */
        printf("Game on!\n");
        printf("Your are %c's\n", id ? 'X' : 'O');
        
        draw_board(board);

        while(1) {
            recv_msg(sockfd, msg);

            if (!strcmp(msg, "TRN")) { 
                printf("Your move...\n");
                take_turn(sockfd);
            }
            else if (!strcmp(msg, "INV")) { 
                printf("That position has already been played. Try again.\n"); 
            }
            else if (!strcmp(msg, "CNT")) { 
                int num_players = recv_int(sockfd);
                printf("There are currently %d active players.\n", num_players); 
            }
            else if (!strcmp(msg, "UPD")) { 
                get_update(sockfd, board);
                draw_board(board);
            }
            else if (!strcmp(msg, "WAT")) { 
                printf("Waiting for other players move...\n");
            }
            else if (!strcmp(msg, "WIN")) { 
                printf("You win!\n");
                break;
            }
             else if (!strcmp(msg, "LSE")) { 
                printf("You lost.\n");
                break;
            }
            else if (!strcmp(msg, "DRW")) { 
                printf("Draw.\n");
                break;
            }
            else 
                error("Unknown message.");
        }

        printf("Game over.\n");
        close(sockfd);
    }

    catch (std::exception& e)
    {
        std::cerr << e.what() << endl;
    }
    return 0;
    
}
