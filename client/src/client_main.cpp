#include <unistd.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <iostream>
#include <iomanip>
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
    
    #ifdef DEBUG
    	cout << "[DEBUG] Received int: " << msg << endl;
	#endif
    
    return msg;
}

void recv_msg(int sockfd, char * msg)
{
    memset(msg, 0, 4);
    int n = read(sockfd, msg, 3);
    
    if (n < 0 || n != 3)
     error("ERROR reading message from server socket.");

    #ifdef DEBUG
    	cout << "[DEBUG] Received message: " << msg << endl;
	#endif
}

void draw_board(char board[3][3])
{
        cout << setw(25) << board[0][0] << " | " << board[0][1] << " | " << board[0][2] << " " << endl;
        cout << setw(33) << "---------" << endl;
        cout << setw(25) << board[1][0] << " | " << board[1][1] << " | " << board[1][2] << " " << endl;
        cout << setw(33) << "---------" << endl;
        cout << setw(25) << board[2][0] << " | " << board[2][1] << " | " << board[2][2] << " " << endl;
}

void write_server_int(int sockfd, int msg)
{
    int n = write(sockfd, &msg, sizeof(int));
    if (n < 0)
        error("ERROR writing int to server socket");
    
    #ifdef DEBUG
    	cout << "[DEBUG] Wrote int to server: " << msg << endl;
	#endif
}

void take_turn(int sockfd)
{
    char buffer[10];

    while (1) {
    cout << "Enter 0-8 to make a move, or 9 for number of active players: ";
        fgets(buffer, 10, stdin);
        int move = buffer[0] - '0';

    if (move <= 9 && move >= 0){
            cout << endl;
            write_server_int(sockfd, move);
            break;
    }

    else
            cout << endl << "Invalid input. Try again." << endl;
    }
}

void get_update(int sockfd, char board[][3])
{
    
    int player_id = recv_int(sockfd);
    int move = recv_int(sockfd);
    board[move/3][move%3] = player_id ? 'X' : 'O';    
}

void design()
{
    char board[3][3] = { {'X', 'O', 'X'},
                            {'X', 'O', 'X'},
                            {'X', 'O', 'X'} };

    cout << "\nWait Client is connecting ..." << endl;
    this_thread::sleep_for(2000ms);
    cout << "Connecting . . . " << endl;

    float progress = 0.0;

    while (progress <= 1.0) {
            int barWidth = 70;

            cout << "[";

            int pos = barWidth * progress;

            for (int i = 0; i < barWidth; ++i) {
                    if (i < pos) cout << "=";
                    else if (i == pos) cout << ">";
                    else cout << " ";
            }

            cout << "] " << int(progress * 100.0) << " %\r";
            cout.flush();
            this_thread::sleep_for(1000ms);
            progress += 0.24;
    }

    this_thread::sleep_for(2000ms);

    system("clear");
    cout << endl;
    cout << setw(50) << "WELCOME TO THE TIC-TAC-TOE SERVER GAME" << endl;
    cout << setw(50) << "--------------------------------------" << endl;
    draw_board(board);
    cout << setw(50) << "--------------------------------------" << endl;
	cout << endl;
}

int main(int argc, char *argv[])
{
    try {
		// validate no of arguments
		if (argc < 3)
		{
		    	cout << "Usage " << argv[0] << " hostname port" << endl;
	    		exit(1);
		}

		design();
		
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
		cout << "[DEBUG] Client ID: " << id << endl;
		#endif

		char msg[4];
		char board[3][3] = { {' ', ' ', ' '},
			             {' ', ' ', ' '},
                                     {' ', ' ', ' '} };

	       	do {
	    		recv_msg(sockfd, msg);
	    		if (!strcmp(msg, "HLD"))
				cout << endl << "Waiting for a second player..." << endl;
		} while ( strcmp(msg, "SRT") );

		/* The game has begun. */
		cout << "Game on!" << endl;
		cout << "You are " << (id ? 'X' : 'O') << "'s" << endl;
		draw_board(board);

		while(1) {
            recv_msg(sockfd, msg);

            if (!strcmp(msg, "TRN")) {
            cout << endl << "Your move..." << endl;
            take_turn(sockfd);
			}
			else if (!strcmp(msg, "INV")) {
            cout << "That position has already been played. Try again." << endl;
            }
            else if (!strcmp(msg, "CNT")) {
            int num_players = recv_int(sockfd);
            cout << "There are currently " << num_players << " active players." << endl;
            }
            else if (!strcmp(msg, "UPD")) {
            get_update(sockfd, board);
            draw_board(board);
            }
            else if (!strcmp(msg, "WAT")) {
            cout << endl << "Waiting for other players move..." << endl;  
			}
            else if (!strcmp(msg, "WIN")) {
            cout << "You win!" << endl;
            break;
            }
	   		else if (!strcmp(msg, "LSE")) {
            cout << "You lost." << endl;
            break;
            }
            else if (!strcmp(msg, "DRW")) {
            cout << "Draw." << endl;
            break;
            }
            else
            error("Unknown message.");
		}
		cout << "Game over." << endl;
		close(sockfd);
    }

    catch (std::exception& e)
    {
    std::cerr << e.what() << std::endl;
    }

    return 0;
}
