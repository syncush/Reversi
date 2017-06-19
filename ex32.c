/************************************************************
 *                          Ex31.c
 *                  Name:   Daniel Hermon
 *                  I.D     209028778
 *                  Group:  03
 *
 * **********************************************************/
#include <zconf.h>
#include <memory.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <stdio.h>
#include <sys/shm.h>

#define KILOBYTE 1024
#define PLEASE_CHOOSE_A_SQUARE "Please choose a square\n"
#define NO_SUCH_SQUARE "No such square\n"
#define THIS_SQUARE_IS_INVALID "This square is invalid\n"
#define PLEASE_CHOOSE_ANOTHER_SQUARE "Please choose another square\n"
#define THE_BOARD_IS "The board is:\n"
#define WAITING_FOR_THE_OTHER_PLAYER "Waiting for the other player to make a move\n"
#define BLACK_POWER "Winning player: Black\n"
#define WHITE_WINS "Winning player: White\n"
#define TIE "No winning player\n"
#define BOARDSIZE 8

char *dataPointer;
int flag = 0;
int gameBoard[BOARDSIZE][BOARDSIZE] = {{0}};
int playerID;
typedef struct PlayerMovement {
    int player;
    int row;
    int col;
} PlayerMove;
typedef struct sigaction Sigaction;
Sigaction act;

void HandleGameOver(int player);

char FromPlayerToChar(int player);

void DoMove(PlayerMove *pMovement);

int MoveValidChecker(PlayerMove *pMovement);

void InitGameBoard();

int PlacementValidChecker(PlayerMove *pMovement);

int GetEnemy(int player);

int GetFirstInUpwards(PlayerMove *pMovement);

int GetFirstDownwards(PlayerMove *pMovement);

int GetFirstLeft(PlayerMove *pMovement);

int GetFirstRight(PlayerMove *pMovement);

void GetDownRightDiagonal(int *row, int *col, PlayerMove *pMovement);

void GetUpperLeftDiagonal(int *row, int *col, PlayerMove *pMovement);

void GetUpperRightDiagonal(int *row, int *col, PlayerMove *pMovement);

void GetDownLeftDiagonal(int *row, int *col, PlayerMove *pMovement);

int IsGameOver(int player);

void PlayGame(char *s);

int PiecesCount(int player);

void PrintGameBoard();

int IsValidInput(int row, int col);

int TranslateToPlayerID(char turn);

void ConnectSHM_SIGUSR1(int sig);

int main(int argc, char *argv[]) {
    int fd_write;
    pid_t myPid;
    InitGameBoard();
    int row, col;
    int rowInput,colInput;
    int flagFull = 1;
    //open fifo
    if ((fd_write = open("fifo_clientTOserver", O_RDWR)) < 0) {
        write(STDERR_FILENO, "failed to open fifo", strlen("failed to open fifo"));
        exit(-1);
    }
    //write to it my pid

    myPid = getpid();
    if (write(fd_write, &myPid, sizeof(pid_t)) < 0) {
        perror("failed to write to fifo");
        exit(-1);
    }
    //close(fd);

    //define a signal handler for SIGUSR1 the server sends
    sigset_t block_mask;
    //block every signal
    sigfillset(&block_mask);
    act.sa_mask = block_mask;
    act.sa_flags = 0;
    act.sa_handler = ConnectSHM_SIGUSR1;
    //activate sigacgion.
    if (sigaction(SIGUSR1, &act, NULL) < 0) {
        perror("Failed to sigaction");
        exit(1);
    }
    pause();
    char userInput[KILOBYTE];
    int flag = 0;
    PlayerMove playerMove;
    int shouldStop = 0;
    int enemy = GetEnemy(playerID);
    write(1, THE_BOARD_IS,strlen(THE_BOARD_IS));
    PrintGameBoard();
    if(playerID == 1) {
        while(dataPointer[0] == '\0') {
            sleep(1);
            write(1, WAITING_FOR_THE_OTHER_PLAYER, strlen(WAITING_FOR_THE_OTHER_PLAYER));
        }
        playerMove.player = TranslateToPlayerID(dataPointer[0]);
        playerMove.row = dataPointer[2] - '0';
        playerMove.col = dataPointer[1] - '0';
        DoMove(&playerMove);
        write(1, THE_BOARD_IS,strlen(THE_BOARD_IS));
        PrintGameBoard();
    }
    //main game loop
    while (!shouldStop) {
        flag = 0;
        while (flag != 1) {

            write(1, PLEASE_CHOOSE_A_SQUARE, strlen(PLEASE_CHOOSE_A_SQUARE));
            scanf("\n[%d,%d]",&colInput,&rowInput);
            int validFlag = IsValidInput(rowInput, colInput);
            if (!validFlag) {
                write(1, NO_SUCH_SQUARE, strlen(NO_SUCH_SQUARE));
                write(1, PLEASE_CHOOSE_ANOTHER_SQUARE, strlen(PLEASE_CHOOSE_ANOTHER_SQUARE));
                flag = 0;
                continue;
            } else {
                playerMove.player = playerID;
                playerMove.row = rowInput;
                playerMove.col = colInput;
                validFlag = MoveValidChecker(&playerMove);
                if (!validFlag) {
                    write(1, THIS_SQUARE_IS_INVALID, strlen(THIS_SQUARE_IS_INVALID));
                    write(1, PLEASE_CHOOSE_ANOTHER_SQUARE, strlen(PLEASE_CHOOSE_ANOTHER_SQUARE));
                    flag = 0;
                    continue;
                }
                flag = 1;
            }
        }
        DoMove(&playerMove);
        write(1, THE_BOARD_IS,strlen(THE_BOARD_IS));
        PrintGameBoard();
        dataPointer[3] = '\0';
        dataPointer[2] = rowInput + '0';
        dataPointer[1] = colInput + '0';
        dataPointer[0] = FromPlayerToChar(playerID);
        HandleGameOver(GetEnemy(playerID));
        while (1) {
            sleep(1);
            write(1, WAITING_FOR_THE_OTHER_PLAYER, strlen(WAITING_FOR_THE_OTHER_PLAYER));
            int enemy = TranslateToPlayerID(dataPointer[0]);
            if (enemy != playerID) {
                int enemyRow = dataPointer[2] - '0';
                int enemyCol = dataPointer[1] - '0';
                playerMove.player = enemy;
                playerMove.row = enemyRow;
                playerMove.col = enemyCol;
                DoMove(&playerMove);
                HandleGameOver(playerID);
                write(1, THE_BOARD_IS, strlen(THE_BOARD_IS));
                PrintGameBoard();
                break;
            }
            if(dataPointer[8] != 0) {
                if(dataPointer[8] == 'w') {
                    write(1, WHITE_WINS, strlen(WHITE_WINS));
                } else {
                    if(dataPointer[8] == 'b') {
                        write(1, BLACK_POWER, strlen(BLACK_POWER));
                    } else {
                        write(1, TIE, strlen(TIE));
                    }
                }
                exit(1);
            }
        }
    }
}

/**
 * Function initalize the board to new game as in the rules of reversi.
 */
void InitGameBoard() {
    memset(gameBoard, 0, sizeof(gameBoard[0][0]) * BOARDSIZE * BOARDSIZE);
    gameBoard[3][3] = 2;
    gameBoard[3][4] = 1;
    gameBoard[4][4] = 2;
    gameBoard[4][3] = 1;
}

/**
 *
 * @param sig
 */
void ConnectSHM_SIGUSR1(int sig) {
    key_t key;
    //create a shared memory key
    if ((key = ftok("ex31.c", 'k')) == -1) {
        perror("ftok");
        exit(0);
    }

    //create a shared memory using that key
    int shmid;
    if ((shmid = shmget(key, KILOBYTE, 0644 | IPC_CREAT)) == -1) {
        perror("Failed at shmget");
        exit(0);
    }

    if ((dataPointer = shmat(shmid, NULL, 0)) == (char *) -1) {
        perror("Failed at shmat");
        exit(0);
    }

    //get player's color
    struct shmid_ds ds;
    //get shared memory information
    if (shmctl(shmid, IPC_STAT, &ds) == -1) {
        perror("shmtcl");
        exit(1);
    }
    if (ds.shm_nattch == 2)
        playerID = 2;
        //else, my color is white
    else
        playerID = 1;

    flag = 1;
}

/**
 * The function returns the char acording to the player id.
 * @param player player id.
 * @return  w if 1, b if 2
 */
char FromPlayerToChar(int player) {
    if (player == 1) {
        return 'w';
    } else {
        return 'b';
    }
}

/**
 *
 * @param player
 */
void HandleGameOver(int player) {
    int flag = IsGameOver(player);
    int myColor = player;
    if (flag == 1) {
        switch (myColor) {
            case 1: {
                write(1, WHITE_WINS, strlen(WHITE_WINS));
                dataPointer[8] = 'w';
                exit(1);
            }
            case 2: {
                write(1, BLACK_POWER, strlen(BLACK_POWER));
                dataPointer[8] = 'b';
                exit(1);
            }
        }
    } else if (flag == -1) {
        switch (myColor) {
            case 1: {
                write(1, BLACK_POWER, strlen(BLACK_POWER));
                dataPointer[8] = 'b';
                exit(1);
            }
                break;
            case 2: {
                write(1, WHITE_WINS, strlen(WHITE_WINS));
                dataPointer[8] = 'w';
                exit(1);
            }

        }

    } else {
        if (flag == 2) {
            write(1, TIE, strlen(TIE));
            dataPointer[8] = 't';
            exit(1);
        }
    }
}

/**
 * The function returns the player id by a given char.
 * @param turn the char of the player.
 * @return 1 for white , 2 for black.
 */
int TranslateToPlayerID(char turn) {
    if (turn == 'w' || turn == 'W') {
        return 1;
    } else {
        if (turn == 'b' || turn == 'B') {
            return 2;
        }
    }
    return -1;
}

/**
 * The function prints the board.
 */
void PrintGameBoard() {
    int row, col;
    char print;
    for (row = 0; row < BOARDSIZE; ++row) {
        for (col = 0; col < BOARDSIZE; ++col) {
            if (gameBoard[row][col] == 1) {
                print = '1';
            } else {
                if (gameBoard[row][col] == 2) {
                    print = '2';
                } else {
                    print = '0';
                }
            }
            write(1, &print, 1);
            write(1, " ", strlen(" "));
        }
        write(1, "\n", strlen("\n"));
    }
}

/**
 * The function checks if a given input is legit.
 * @param inp the input string.
 * @return  1 for valid , 0 for unvalid.
 */
int IsValidInput(int row, int col) {
        if (row < 0 || row > 7 || col < 0 || col > 7) {
            return 0;
        } else {
            return 1;
        }
}

/**
 * The function checks if the game ended.
 * @param player id of the player.
 * @return 0 if continue , 1 for player won , -1 if enemy won. 2 tie
 */
int IsGameOver(int player) {
    int enemyPiecesCount = PiecesCount(GetEnemy(player));
    int myPiecesCount = PiecesCount(player);
    int row, col;
    int flagFull = 1;
    for (row = 0; row < BOARDSIZE; row++) {
        for (col = 0; col < BOARDSIZE; col++) {
            if (gameBoard[row][col] == 0) {
                flagFull = 0;
                break;
            }
        }
    }
    if(flagFull) {
        if (enemyPiecesCount > myPiecesCount) {
            return -1;
        } else if (myPiecesCount > enemyPiecesCount) {
            return 1;

        } else {
            return 2;
        }
    }
    int isAllBlack = 1;
    int isAllWhite = 1;
    for (row = 0; row < BOARDSIZE; row++) {
        for (col = 0; col < BOARDSIZE; col++) {
                if(gameBoard[row][col] == 1) {
                    isAllBlack = 0;
                }
                if(gameBoard[row][col] == 2) {
                    isAllWhite = 0;
                }

        }
    }
    if(isAllWhite && !isAllBlack) {
        if(player == 1) {
            return 1;
        } else {
            return -1;
        }
    }
    if(!isAllWhite && isAllBlack) {
        if(player == 1) {
            return -1;
        } else {
            return 1;
        }
    }
   // return 0;
    PlayerMove tempMovement;
    tempMovement.player = player;
    for (row = 0; row < BOARDSIZE; row++) {
        for (col = 0; col < BOARDSIZE; col++) {
            if (gameBoard[row][col] == 0) {
                tempMovement.row = row;
                tempMovement.col = col;
                int flag = MoveValidChecker(&tempMovement);
                if (flag) {
                    return 0;
                }
            }
        }
    }
        if (enemyPiecesCount > myPiecesCount) {
            return -1;
        } else if (myPiecesCount > enemyPiecesCount) {
            return 1;

        } else {
            return 2;
        }
}

/**
 * The function gets a player id and returns how many pieces he got on the board.
 * @param player player id.
 * @return number of pieces he has on the board.
 */
int PiecesCount(int player) {
    if (player == 2 || player == 1) {
        int count = 0;
        int i, j;
        for (i = 0; i < BOARDSIZE; i++) {
            for (j = 0; j < BOARDSIZE; j++) {
                if (gameBoard[i][j] == player) {
                    ++count;
                }
            }
        }
        return count;
    } else {
        return -1;
    }
}

/**
 * The function makes the requested movement
 * @param pMovement struct holds movement info.
 */
void DoMove(PlayerMove *pMovement) {
    int result = MoveValidChecker(pMovement);
    if (result) {
        gameBoard[pMovement->row][pMovement->col] = pMovement->player;
        int enemy = GetEnemy(pMovement->player);
        int tempRow, tempCol, flag;
        tempRow = GetFirstInUpwards(pMovement);
        //Upwards
        while (tempRow < pMovement->row && tempRow >= 0) {
            gameBoard[tempRow][pMovement->col] = pMovement->player;
            tempRow++;
        }
        int i = GetFirstDownwards(pMovement);
        tempRow = pMovement->row;
        //Downwards
        while (tempRow <= i && tempRow >= 0) {
            gameBoard[tempRow][pMovement->col] = pMovement->player;
            tempRow++;
        }
        tempCol = GetFirstLeft(pMovement);
        //Left
        while (tempCol < pMovement->col && tempCol >= 0) {
            gameBoard[pMovement->row][tempCol] = pMovement->player;
            tempCol++;
        }
        tempCol = GetFirstRight(pMovement);
        i = pMovement->col;
        //Right
        while (i <= tempCol && tempCol >= 0) {
            gameBoard[pMovement->row][i] = pMovement->player;
            i++;
        }
        /**************Diagonals**********/
        GetDownRightDiagonal(&tempRow, &tempCol, pMovement);
        int j = pMovement->col;
        i = pMovement->row;
        while (i < tempRow && j < tempCol && tempCol >= 0 && tempRow >= 0) {
            gameBoard[i][j] = pMovement->player;
            i++;
            j++;
        }
        GetUpperLeftDiagonal(&tempRow, &tempCol, pMovement);
        while (tempRow >= 0 && tempCol >= 0 && tempRow < pMovement->row && tempCol < pMovement->col) {
            tempRow++;
            tempCol++;
            gameBoard[tempRow][tempCol] = pMovement->player;
        }
        GetUpperRightDiagonal(&tempRow, &tempCol, pMovement);
        while (tempRow >= 0 && tempCol >= 0 && tempRow < pMovement->row && tempCol > pMovement->col) {
            gameBoard[tempRow][tempCol] = pMovement->player;
            tempRow++;
            tempCol--;
        }
        GetDownLeftDiagonal(&tempRow, &tempCol, pMovement);
        while (tempRow >= 0 && tempCol >= 0 && tempRow > pMovement->row && tempCol < pMovement->col) {
            gameBoard[tempRow][tempCol] = pMovement->player;
            tempRow--;
            tempCol++;
        }
    } else {
        return;
    }
}

/**
 * The function gets the indexes of the down left diagonal according to the movement.
 * @param row parameter the func sets. -1 if not exist.
 * @param col parameter the func sets, -1 if not exist.
 * @param pMovement the struct holds the info about the movement.
 */
void GetDownLeftDiagonal(int *row, int *col, PlayerMove *pMovement) {
    *row = -1;
    *col = -1;
    int tempRow = pMovement->row + 1;
    int tempCol = pMovement->col - 1;
    while (tempRow < BOARDSIZE && tempCol >= 0) {
        if (gameBoard[tempRow][tempCol] == pMovement->player) {
            *row = tempRow;
            *col = tempCol;
            break;
        } else {
            if(gameBoard[tempRow][tempCol] == 0) {
                break;
            }
        }
        tempRow++;
        tempCol--;
    }
}

/**
 * The function gets the indexes of the upper right diagonal according to the movement.
 * @param row parameter the func sets. -1 if not exist.
 * @param col parameter the func sets, -1 if not exist.
 * @param pMovement the struct holds the info about the movement.
 */
void GetUpperRightDiagonal(int *row, int *col, PlayerMove *pMovement) {
    *row = -1;
    *col = -1;
    int tempRow = pMovement->row - 1;
    int tempCol = pMovement->col + 1;
    while (tempRow >= 0 && tempCol < BOARDSIZE) {
        if (gameBoard[tempRow][tempCol] == pMovement->player) {
            *row = tempRow;
            *col = tempCol;
            break;
        } else {
            if(gameBoard[tempRow][tempCol] == 0) {
                break;
            }
        }
        tempRow--;
        tempCol++;
    }
}

/**
 * The function gets the indexes of the upper left diagonal according to the movement.
 * @param row parameter the func sets. -1 if not exist.
 * @param col parameter the func sets, -1 if not exist.
 * @param pMovement the struct holds the info about the movement.
 */
void GetUpperLeftDiagonal(int *row, int *col, PlayerMove *pMovement) {
    *row = -1;
    *col = -1;
    int tempRow = pMovement->row - 1;
    int tempCol = pMovement->col - 1;
    while (tempRow >= 0 && tempCol >= 0) {
        if (gameBoard[tempRow][tempCol] == pMovement->player) {
            *row = tempRow;
            *col = tempCol;
            break;
        } else {
            if(gameBoard[tempRow][tempCol] == 0) {
                break;
            }
        }
        tempRow--;
        tempCol--;
    }
}

/**
 * The function gets the indexes of the down right diagonal according to the movement.
 * @param row parameter the func sets. -1 if not exist.
 * @param col parameter the func sets, -1 if not exist.
 * @param pMovement the struct holds the info about the movement.
 */
void GetDownRightDiagonal(int *row, int *col, PlayerMove *pMovement) {
    int tempRow = pMovement->row + 1;
    int tempCol = pMovement->col + 1;
    *row = -1;
    *col = -1;
    while (tempRow < BOARDSIZE && tempCol < BOARDSIZE) {
        if (gameBoard[tempRow][tempCol] == pMovement->player) {
            *row = tempRow;
            *col = tempCol;
            break;
        } else {
            if(gameBoard[tempRow][tempCol] == 0) {
                break;
            }
        }
        tempRow++;
        tempCol++;
    }
}

/**
 * The function gets the index row of the first player piece.
 * @param pMovement the struct holds the info about the movement.
 * @return the index of row of the first player piece.
 */
int GetFirstRight(PlayerMove *pMovement) {
    int i = 0;
    int flag = 0;
    int result = -1;
    for (i = pMovement->col + 1; i < BOARDSIZE; ++i) {
        if (gameBoard[pMovement->row][i] == pMovement->player) {
            flag = 1;
            result = i;
            break;
        } else {
            if(gameBoard[pMovement->row][i]  == 0) {
                break;
            }
        }
    }
    return result;
}

/**
 * The function gets the left row index of the first player piece.
 * @param pMovement the struct holds the info about the movement.
 * @return the index of left row of the first player piece.
 */
int GetFirstLeft(PlayerMove *pMovement) {
    int i = pMovement->col - 1;
    int flag = 0;
    int result = -1;
    for (i = pMovement->col - 1; i >= 0; --i) {
        if (gameBoard[pMovement->row][i] == pMovement->player) {
            flag = 1;
            result = i;
            break;
        } else {
            if(gameBoard[pMovement->row][i]  == 0) {
                break;
            }
        }
    }
    return result;
}

/**
 * The function gets the right row index of the first player piece.
 * @param pMovement the struct holds the info about the movement.
 * @return the index of right row of the first player piece.
 */
int GetFirstDownwards(PlayerMove *pMovement) {
    int i = 0;
    int flag = 0;
    int result = -1;
    for (i = pMovement->row + 1; i < BOARDSIZE; ++i) {
        if (gameBoard[i][pMovement->col] == pMovement->player) {
            flag = 1;
            result = i;
            break;
        } else {
            if(gameBoard[i][pMovement->col]  == 0) {
                break;
            }
        }
    }
    return result;
}

/**
 * The function gets the left col index of the first player piece.
 * @param pMovement the struct holds the info about the movement.
 * @return the index of left col of the first player piece.
 */
int GetFirstInUpwards(PlayerMove *pMovement) {
    int i = 0;
    int flag = 0;
    int result = -1;
    for (i = pMovement->row - 1; i >= 0; --i) {
        if (gameBoard[i][pMovement->col] == pMovement->player) {
            flag = 1;
            result = i;
            break;
        } else {
            if(gameBoard[i][pMovement->col]  == 0) {
                break;
            }
        }
    }
    return result;
}

/**
 * The function returns the enemy player id.
 * @param player current player id.
 * @return his enemy , if 2 returns 1 , if 1 returns 2.
 */
int GetEnemy(int player) {
    return 3 - player;
}

/**
 * Checks if movement is valid.
 * @param pMovement info of the movement.
 * @return 1 for can move , 0 for no.
 */
int MoveValidChecker(PlayerMove *pMovement) {
    if (pMovement->row < 0 || pMovement->col < 0 || pMovement->row >= 8 || pMovement->col >= 8 ||
        gameBoard[pMovement->row][pMovement->col] != 0) {
        return 0;
    } else {
        return PlacementValidChecker(pMovement);
    }

}

/**
 * Function checks if board piece placement is valid.
 * @param pMovement struct info holds of movement.
 * @return 0 if no 1 if valid placement.
 */
int PlacementValidChecker(PlayerMove *pMovement) {
    int enemy;
    int temp;
    int flag = 0;
    int isBetweenFlag = 0;
    int tempLine, tempColum;
    //pMovement->player == 1? enemy = 2:enemy = 1;
    enemy = GetEnemy(pMovement->player);
    temp = pMovement->col + 1;
    //Forwards
    while (temp < BOARDSIZE) {
        if (gameBoard[pMovement->row][temp] == enemy) {
            flag = 1;
            temp++;
        } else {
            if (gameBoard[pMovement->row][temp] == pMovement->player) {
                isBetweenFlag = 1;
                break;
            }
            if (gameBoard[pMovement->row][temp] == 0) {
                break;
            }
        }
    }
    if (flag && isBetweenFlag && temp < BOARDSIZE) {
        return 1;
    }
    //Backwards
    isBetweenFlag = 0;
    flag = 0;
    temp = pMovement->col - 1;
    while (temp >= 0) {
        if (gameBoard[pMovement->row][temp] == enemy) {
            flag = 1;
            temp--;
        } else {
            if (gameBoard[pMovement->row][temp] == pMovement->player) {
                isBetweenFlag = 1;
                break;
            }
            if (gameBoard[pMovement->row][temp] == 0) {
                break;
            }
        }
    }
    if (flag && isBetweenFlag && temp >= 0) {
        return 1;
    }
    //DownWards
    flag = 0;
    isBetweenFlag = 0;
    temp = pMovement->row + 1;
    while (temp < BOARDSIZE) {
        if (gameBoard[temp][pMovement->col] == enemy) {
            flag = 1;
            temp++;
        } else {
            if (gameBoard[temp][pMovement->col] == pMovement->player) {
                isBetweenFlag = 1;
                break;
            }
            if (gameBoard[temp][pMovement->col] == 0) {
                break;
            }
        }
    }
    if (flag &&  isBetweenFlag && temp < BOARDSIZE) {
        return 1;
    }
    //Upwards
    flag = 0;
    isBetweenFlag = 0;
    temp = pMovement->row - 1;
    while (temp >= 0) {
        if (gameBoard[temp][pMovement->col] == enemy) {
            flag = 1;
            temp--;
        } else {
            if (gameBoard[temp][pMovement->col] == pMovement->player) {
                isBetweenFlag = 1;
                break;
            }
            if (gameBoard[temp][pMovement->col] == 0) {
                break;
            }
        }
    }
    if (flag && isBetweenFlag && temp >= 0) {
        return 1;
    }
    /*****************Check Diagonals.*****************/
    //DownRight Diagonal
    flag = 0;
    isBetweenFlag = 0;
    tempLine = pMovement->row + 1;
    tempColum = pMovement->col + 1;
    while (tempLine < BOARDSIZE && tempColum < BOARDSIZE) {
        if (gameBoard[tempLine][tempColum] == enemy) {
            flag = 1;
            tempLine++;
            tempColum++;
        } else {
            if (gameBoard[tempLine][tempColum] == pMovement->player) {
                isBetweenFlag = 1;
                break;
            }
            if (gameBoard[tempLine][tempColum] == 0) {
                break;
            }
        }
    }
    if (tempLine < BOARDSIZE && tempColum < BOARDSIZE && flag && isBetweenFlag) {
        return 1;
    }
    //Left upper diagonal.
    flag = 0;
    isBetweenFlag = 0;
    tempLine = pMovement->row - 1;
    tempColum = pMovement->col - 1;
    while (tempLine >= 0 && tempColum >= 0) {
        if (gameBoard[tempLine][tempColum] == enemy) {
            flag = 1;
            tempLine--;
            tempColum--;
        } else {
            if (gameBoard[tempLine][tempColum] == pMovement->player) {
                isBetweenFlag = 1;
                break;
            }
            if (gameBoard[tempLine][tempColum] == 0) {
                break;
            }
        }
    }
    if (tempLine >= 0 && tempColum >= 0 && flag && isBetweenFlag) {
        return 1;
    }
    //Upper right diagonal.
    flag = 0;
    isBetweenFlag = 0;
    tempLine = pMovement->row - 1;
    tempColum = pMovement->col + 1;
    while (tempLine >= 0 && tempColum < BOARDSIZE) {
        if (gameBoard[tempLine][tempColum] == enemy) {
            flag = 1;
            tempLine--;
            tempColum++;
        } else {
            if (gameBoard[tempLine][tempColum] == pMovement->player) {
                isBetweenFlag = 1;
                break;
            }
            if (gameBoard[tempLine][tempColum] == 0) {
                break;
            }
        }
    }
    if (tempLine >= 0 && tempColum < BOARDSIZE && flag && isBetweenFlag) {
        return 1;
    }
    //Down left diagonal
    flag = 0;
    isBetweenFlag = 0;
    tempLine = pMovement->row + 1;
    tempColum = pMovement->col - 1;
    while (tempLine < BOARDSIZE && tempColum >= 0) {
        if (gameBoard[tempLine][tempColum] == enemy) {
            flag = 1;
            tempLine++;
            tempColum--;
        } else {
            if (gameBoard[tempLine][tempColum] == pMovement->player) {
                isBetweenFlag = 1;
                break;
            }
            if (gameBoard[tempLine][tempColum] == 0) {
                break;
            }
        }
    }
    if (tempLine < BOARDSIZE && tempColum >= 0 && flag && isBetweenFlag) {
        return 1;
    }
    return 0;
}
