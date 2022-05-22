#include <ctime>
#include <iostream>
#include <string>
#include <asio.hpp>
#include <iomanip>
#include <thread>
#include <fstream>

using namespace std;
using asio::ip::tcp;

int player_count = 0;
pthread_mutex_t mutexcount;

void error(const char *msg)
{
    perror(msg);
    exit(1);
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

    #ifdef DEBUG
	cout << "[DEBUG] Received int: " << msg << endl;
	#endif
    
    return msg;
}


int get_player_move(int cli_sockfd)
{
    #ifdef DEBUG
    cout << "[DEBUG] Getting player move..." << endl;
	#endif
    
	write_client_msg(cli_sockfd, "TRN");

    return recv_int(cli_sockfd);
}


int check_move(char board[][3], int move, int player_id)
{
    if ((move == 9) || (board[move/3][move%3] == ' ')) { 
        
        #ifdef DEBUG
        cout << "[DEBUG] Player " << player_id << "'s move was valid." << endl;
        #endif
        
        return 1;
   }
   else {       
       #ifdef DEBUG
       cout << "[DEBUG] Player " << player_id << "'s move was invalid." << endl;
       #endif
    
       return 0;
   }
}


void update_board(char board[][3], int move, int player_id)
{
    board[move/3][move%3] = player_id ? 'X' : 'O';
    
    #ifdef DEBUG
    cout << "[DEBUG] Board updated." << endl;
    #endif
}


void draw_board(char board[][3])
{
    cout << setw(25) << board[0][0] << " | " << board[0][1] << " | " << board[0][2] << " " << endl;
    cout << setw(33) << "---------" << endl;
    cout << setw(25) << board[1][0] << " | " << board[1][1] << " | " << board[1][2] << " " << endl;
    cout << setw(33) << "---------" << endl;
    cout << setw(25) << board[2][0] << " | " << board[2][1] << " | " << board[2][2] << " " << endl;
}


void send_update(int * cli_sockfd, int move, int player_id)
{
    #ifdef DEBUG
    cout << "[DEBUG] Sending update..." << endl;
    #endif
    
    write_clients_msg(cli_sockfd, "UPD");

    write_clients_int(cli_sockfd, player_id);
    
    write_clients_int(cli_sockfd, move);
    
    #ifdef DEBUG
    cout << "[DEBUG] Update sent." << endl;
    #endif
}

void send_player_count(int cli_sockfd)
{
    write_client_msg(cli_sockfd, "CNT");
    write_client_int(cli_sockfd, player_count);

    #ifdef DEBUG
    cout << "[DEBUG] Player Count Sent." << endl;
    #endif
}

int check_board(char board[][3], int last_move)
{
    #ifdef DEBUG
    cout << "[DEBUG] Checking for a winner..." << endl;
    #endif
   
    int row = last_move/3;
    int col = last_move%3;

    if ( board[row][0] == board[row][1] && board[row][1] == board[row][2] ) { 
        #ifdef DEBUG
       cout << "[DEBUG] Win by row " << row <<"." << endl;
        #endif 
        return 1;
    }
    else if ( board[0][col] == board[1][col] && board[1][col] == board[2][col] ) { 
        #ifdef DEBUG
        cout << "[DEBUG] Win by column " << col << "." << endl;
        #endif 
        return 1;
    }
    else if (!(last_move % 2)) { 
        if ( (last_move == 0 || last_move == 4 || last_move == 8) && (board[1][1] == board[0][0] && board[1][1] == board[2][2]) ) {  
            #ifdef DEBUG
            cout << "[DEBUG] Win by backslash diagonal." << endl;
            #endif 
            return 1;
        }
        if ( (last_move == 2 || last_move == 4 || last_move == 6) && (board[1][1] == board[0][2] && board[1][1] == board[2][0]) ) { 
            #ifdef DEBUG
            cout << "[DEBUG] Win by frontslash diagonal." << endl;
            #endif 
            return 1;
        }
    }

    #ifdef DEBUG
    printf("[DEBUG] No winner, yet.\n");
    #endif
    
    return 0;
}


void *run_game(void *thread_data)
{
    int *cli_sockfd = (int*)thread_data;

    char board[3][3] = { {' ', ' ', ' '},  
                         {' ', ' ', ' '}, 
                         {' ', ' ', ' '} };

    printf("Game on!\n");
    
    write_clients_msg(cli_sockfd, "SRT");

    #ifdef DEBUG
    cout << "[DEBUG] Sent start message." << endl;
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
            cout << endl << "Player " << player_turn << " played position " << move << endl;
                
            valid = check_move(board, move, player_turn);
            if (!valid) { 
                cout << "Move was invalid. Let's try this again..." << endl;
                write_client_msg(cli_sockfd[player_turn], "INV");
            }
        }

	    if (move == -1) { 
            cout << "Player disconnected." << endl;
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
                cout << "Player " << player_turn << " won." << endl;
            }
            else if (turn_count == 8) {                
                cout << "Draw." << endl;
                write_clients_msg(cli_sockfd, "DRW");
                game_over = 1;
            }

            prev_player_turn = player_turn;
            player_turn = (player_turn + 1) % 2;
            turn_count++;
        }
    }

    cout << "Game over." << endl;

	close(cli_sockfd[0]);
    close(cli_sockfd[1]);

    pthread_mutex_lock(&mutexcount);
    player_count--;
    printf("Number of players is now %d.\n", player_count);
    player_count--;
    printf("Number of players is now %d.\n", player_count);
    pthread_mutex_unlock(&mutexcount);
    
    delete []cli_sockfd;

    pthread_exit(NULL);
}

void design()
{
    char board[3][3] = { {'X', 'O', 'X'},
                            {'O', 'X', 'O'},
                            {'O', ' ', 'X'} };

    cout << "\nWait for Server to load..!!" << endl;
	this_thread::sleep_for(2000ms);
    cout << "Loading . . . " << endl;

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
        progress += 0.16;
    }

	this_thread::sleep_for(2000ms);

    system("clear");
	cout << endl;
    cout << setw(50) << "WELCOME TO THE TIC-TAC-TOE SERVER GAME" << endl;
    cout << setw(50) << "--------------------------------------" << endl;
    draw_board(board);
    cout << setw(50) << "--------------------------------------" << endl;
    cout << endl << "Wait for Clients to connect..!!" << endl << endl;
}

int main(int argc, char *argv[])
{
  try {
		int portno;

        std::ifstream cFile ("../config_file.txt");
        if (cFile.is_open())
        {
            std::string line;
            while(getline(cFile, line)){
                line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());
                if(line[0] == '#' || line.empty())
                    continue;
                auto delimiterPos = line.find("=");
                auto name = line.substr(0, delimiterPos);
                auto value = line.substr(delimiterPos + 1);
                std::cout << "server is running on " << name << ": " << value << '\n';

                if(name == "portno") portno = std::stoi(value);
            }
            
        }
        else {
            std::cerr << "Couldn't open config file for reading.\n";
        }
		
		design();
        asio::io_context io_context;
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), portno));
	    
		pthread_mutex_init(&mutexcount, NULL); 
	   
		while(1) {
			if (player_count <= 1){
				int *cli_sockfd = new int[2*sizeof(int)];
		    	memset(cli_sockfd, 0, 2*sizeof(int));
		    
				int num_conn = 0;
                while(num_conn < 2 && player_count <= 1) {					
                    tcp::socket *ptr_socket = new tcp::socket(io_context);
                
                    #ifdef DEBUG
                        cout << "[DEBUG] Listening for clients..." << endl;
                    #endif
            
                    acceptor.accept(*ptr_socket);
                    cli_sockfd[num_conn] = ptr_socket->native_handle();
            
                    if (cli_sockfd[num_conn] < 0)
                            error("ERROR accepting a connection from a client.");

                    #ifdef DEBUG
                    cout << "[DEBUG] Accepted connection from client " << num_conn << endl;
                    #endif
            
                    write(cli_sockfd[num_conn], &num_conn, sizeof(int));

                    #ifdef DEBUG
                    cout << "[DEBUG] Sent client " << num_conn << " it's ID." << endl; 
                    #endif 
                
                    pthread_mutex_lock(&mutexcount);
                    player_count++;
                    cout << "Number of players is now " << player_count << "." << endl;
                    pthread_mutex_unlock(&mutexcount);

                    if (num_conn == 0) {
                            write_client_msg(cli_sockfd[0],"HLD");
                    
                                #ifdef DEBUG
                        cout << "[DEBUG] Told client 0 to hold." << endl;
                                #endif 
                    }	
            
                    num_conn++;
                }

                #ifdef DEBUG
                cout << "[DEBUG] Starting new game thread..." << endl;
                #endif

			    std::thread thread1(run_game, (void*)cli_sockfd);
			    thread1.detach();

			    cout << "New game thread started." << endl;
			}
	    }
    }
    catch (std::exception& e) {
    std::cerr << e.what() << std::endl;
    }

    pthread_mutex_destroy(&mutexcount);

    return 0;
}

