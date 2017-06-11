/************************************************************
 *                          Ex31.c
 *                  Name:   Daniel Hermon
 *                  I.D     209028778
 *                  Group:  03
 *
 * **********************************************************
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <zconf.h>
#include <sys/stat.h>
#include <memory.h>
#include <signal.h>
#include <wait.h>
#include <fcntl.h>

#define BOARDSIZE 8
#define KILOBYTE 1024
char fifoID[1024] = {0};
int gameBoard[BOARDSIZE][BOARDSIZE] = {{0}};
int memSizeOfBoard;

typedef struct PlayerMovement {
    int row;
    int col;
    int player;

} PlayerMove;

void CreateKey(key_t *pInt, char string[7], int i);

void GenerateFifo();

void ReadPlayerPid(pid_t *pInt, int fd);

void ReleaseFifo(int *pInt);

void CreateSharedMemmoryBlock(key_t *pInt, int *pInt1, char *shm);

void InitGameBoard();

void FreeSharedMemmory(char *dataPointer, int *shmid);

void ReadMoveFromSharedMemmory(PlayerMove *pMovement, char *pointer);

void DoMove(PlayerMove *pMovement);

int MoveValidChecker(PlayerMove *pMovement);

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

int TranslateToPlayerID(char turn);

void RemoveSharedMemmory(char *pointer, int shmid);

int main() {
    memSizeOfBoard = sizeof(gameBoard[0][0]) * BOARDSIZE * BOARDSIZE;
    InitGameBoard();
    pid_t firstPlayer;
    pid_t secondPlayer;
    int fifoDescriptor = 0;
    int shmid;
    char *dataPointer = 0;
    key_t key;
    PlayerMove playerMove;
    strcpy(fifoID, "fifo_clientTOserver");
    CreateKey(&key, "ex31.c", 'k');
    //CreateSharedMemmoryBlock(&key, &shmid, dataPointer);
    /*
   * Create the segment.
   */
    if ((shmid = shmget(key, KILOBYTE, IPC_CREAT | 0666)) < 0) {
        perror("shmeget failed");
        exit(1);
    }

    /*
     * Now we attach the segment to our data space.
     */
    if ((dataPointer = shmat(shmid, NULL, 0)) == (char *) -1) {
        perror("shmat failed");
        exit(1);
    }
    memset(dataPointer, 0, KILOBYTE);
    //GenerateFifo();
    /* if (mkfifo(fifoID, 0666) < 0) {
         perror("failed to mkfifo");
     }
     if ((fifoDescriptor = open(fifoID, O_RDWR) < 0)) {
         perror("failed to open fifo server");
         exit(-1);
     }
     read(fifoDescriptor,&firstPlayer,sizeof(pid_t));*/
    //create fifo
    int file = mkfifo("fifo_clientTOserver", 0666);
    if (file < 0) {
        perror("Unable to create a fifo");
        exit(-1);
    }
    int fd_read;
    //open fifo
    if ((fd_read = open("fifo_clientTOserver", O_RDWR)) < 0) {
        perror("Unable to open a fifo");
        exit(-1);
    }
    pid_t firstGivenPid=0,secondGivenPid = 0;
    //get the pid's
    if (read(fd_read, &firstGivenPid, sizeof(pid_t)) < 0) {

        //todo handle
    }
    if (read(fd_read, &secondGivenPid, sizeof(pid_t)) < 0) {
        //todo handle
    }
    ReleaseFifo(&fifoDescriptor);
    memset(dataPointer, 0, KILOBYTE);
    kill(firstGivenPid, SIGUSR1);
    sleep(1);
    kill(secondGivenPid, SIGUSR1);
    while(dataPointer[8] == 0) {
        sleep(2);
    }
    write(1,"GAME OVER\n", strlen("GAME OVER\n"));
    RemoveSharedMemmory(dataPointer, shmid);
    exit(0);
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
 * Function remove shared mem.
 * @param pointer
 * @param shmid
 */
void RemoveSharedMemmory(char *pointer, int shmid) {
    //detach from the shared memory and finish
    if (shmdt(pointer) < 0) {
        perror("Failed to shmdt");
        exit(1);
    }
    if (shmctl(shmid, IPC_RMID, NULL) < 0) {
        perror("shmtcl failed");
        exit(1);
    }
    return;
}

/**
 * The main loop of the game waits for movements.
 * @param data shared memmory pointer
 */
void PlayGame(char *data) {
    int currentTurn = 1;
    int row, col;
    int translatedCharToPlayer = -1;
    PlayerMove playerMove;
    do {
        char playerTurn = data[0];
        translatedCharToPlayer = TranslateToPlayerID(playerTurn);
        if (translatedCharToPlayer == -1) {
            continue;
        }
        if (translatedCharToPlayer != currentTurn) {
            currentTurn = translatedCharToPlayer;
            row = data[1] - '0';
            col = data[2] - '0';
            playerMove.player = translatedCharToPlayer;
            playerMove.row = row;
            playerMove.col = col;
            DoMove(&playerMove);
        }
    } while (!IsGameOver(translatedCharToPlayer));
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
 * The function checks if the game ended.
 * @param player id of the player.
 * @return 0 if continue , 1 for player won , -1 if enemy won.
 */
int IsGameOver(int player) {
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
    if (flagFull) {
        return 1;
    }
    PlayerMove tempMovement;
    tempMovement.player = GetEnemy(player);
    for (row = 0; row < BOARDSIZE; row++) {
        for (col = 0; col < BOARDSIZE; col++) {
            if (gameBoard[row][col] == 0) {
                tempMovement.row = row;
                tempMovement.col = col;
                int flag = MoveValidChecker(&tempMovement);
                if (flag) {
                    return 1;
                }

            }
        }
    }
    int enemyPiecesCount = PiecesCount(GetEnemy(player));
    int myPiecesCount = PiecesCount(player);
    if (enemyPiecesCount > myPiecesCount) {
        return -1;
    } else if (myPiecesCount > enemyPiecesCount) {
        return 1;

    } else {
        return 0;
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
        while (tempRow >= 0 && tempCol >= 0 && tempRow < pMovement->row && tempCol > pMovement->col) {
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
            if (gameBoard[pMovement->row][temp] == pMovement->player || gameBoard[pMovement->row][temp] == 0) {
                break;
            }
        }
    }
    if (flag && temp < BOARDSIZE) {
        return 1;
    }
    //Backwards
    flag = 0;
    temp = pMovement->col - 1;
    while (temp >= 0) {
        if (gameBoard[pMovement->row][temp] == enemy) {
            flag = 1;
            temp--;
        } else {
            if (gameBoard[pMovement->row][temp] == pMovement->player || gameBoard[pMovement->row][temp] == 0) {
                break;
            }
        }
    }
    if (flag && temp >= 0) {
        return 1;
    }
    //DownWards
    flag = 0;
    temp = pMovement->row + 1;
    while (temp < BOARDSIZE) {
        if (gameBoard[temp][pMovement->col] == enemy) {
            flag = 1;
            temp++;
        } else {
            if (gameBoard[pMovement->row][temp] == pMovement->player || gameBoard[pMovement->row][temp] == 0) {
                break;
            }
        }
    }
    if (flag && temp < BOARDSIZE) {
        return 1;
    }
    //Upwards
    flag = 0;
    temp = pMovement->row - 1;
    while (temp >= 0) {
        if (gameBoard[temp][pMovement->col] == enemy) {
            flag = 1;
            temp--;
        } else {
            if (gameBoard[pMovement->row][temp] == pMovement->player || gameBoard[pMovement->row][temp] == 0) {
                break;
            }
        }
    }
    if (flag && temp >= 0) {
        return 1;
    }
    /*****************Check Diagonals.*****************/
    //DownRight Diagonal
    flag = 0;
    tempLine = pMovement->row + 1;
    tempColum = pMovement->col + 1;
    while (tempLine < BOARDSIZE && tempColum < BOARDSIZE) {
        if (gameBoard[tempLine][tempColum] == enemy) {
            flag = 1;
            tempLine++;
            tempColum++;
        } else {
            if (gameBoard[tempLine][tempColum] == pMovement->player || gameBoard[tempLine][tempColum] == 0) {
                break;
            }
        }
    }
    if (tempLine < BOARDSIZE && tempColum < BOARDSIZE && flag) {
        return 1;
    }
    //Left upper diagonal.
    flag = 0;
    tempLine = pMovement->row - 1;
    tempColum = pMovement->col - 1;
    while (tempLine >= 0 && tempColum >= 0) {
        if (gameBoard[tempLine][tempColum] == enemy) {
            flag = 1;
            tempLine--;
            tempColum--;
        } else {
            if (gameBoard[tempLine][tempColum] == pMovement->player || gameBoard[tempLine][tempColum] == 0) {
                break;
            }
        }
    }
    if (tempLine >= 0 && tempColum >= 0 && flag) {
        return 1;
    }
    //Upper right diagonal.
    flag = 0;
    tempLine = pMovement->row - 1;
    tempColum = pMovement->col + 1;
    while (tempLine >= 0 && tempColum < BOARDSIZE) {
        if (gameBoard[tempLine][tempColum] == enemy) {
            flag = 1;
            tempLine--;
            tempColum++;
        } else {
            if (gameBoard[tempLine][tempColum] == pMovement->player || gameBoard[tempLine][tempColum] == 0) {
                break;
            }
        }
    }
    if (tempLine >= 0 && tempColum < BOARDSIZE && flag) {
        return 1;
    }
    //Down left diagonal
    flag = 0;
    tempLine = pMovement->row - 1;
    tempColum = pMovement->col - 1;
    while (tempLine < BOARDSIZE && tempColum >= 0) {
        if (gameBoard[tempLine][tempColum] == enemy) {
            flag = 1;
            tempLine++;
            tempColum--;
        } else {
            if (gameBoard[tempLine][tempColum] == pMovement->player || gameBoard[tempLine][tempColum] == 0) {
                break;
            }
        }
    }
    if (tempLine < BOARDSIZE && tempColum >= 0 && flag) {
        return 1;
    }
    return 0;
}

/**
 * The function reads a player movement from shared memmory.
 * @param pMovement movement info struct container.
 * @param pointer pointer to the shared memmory.
 */
void ReadMoveFromSharedMemmory(PlayerMove *pMovement, char *pointer) {
    while (*pointer == '\0');
    pMovement->player = (pointer[0] == 'w' || pointer[0] == 'W') ? 1 : 2;
    pMovement->col = pointer[1] - 32;
    pMovement->row = pointer[2] - 32;
    //memset(pointer, 0, KILOBYTE);
}

/**
 * Free the shared memmory allocated resources.
 * @param dataPointer pointer to the data.
 * @param shmid id of the shared memmory.
 */
void FreeSharedMemmory(char *dataPointer, int *shmid) {
    if (shmdt(dataPointer) < 0) {
        perror("Failed to shmdt");
        exit(1);
    }
    if (shmctl(*shmid, IPC_RMID, NULL) < 0) {
        perror("Failed to shmtcl");
        exit(1);
    }
}

/**
 * Function initalize the board to new game as in the rules of reversi.
 */
void InitGameBoard() {
    memset(gameBoard, 0, memSizeOfBoard);
    gameBoard[3][3] = 2;
    gameBoard[3][4] = 1;
    gameBoard[4][4] = 2;
    gameBoard[4][3] = 1;
}

void CreateSharedMemmoryBlock(key_t *key, int *shmid, char *shm) {

    /*
     * Create the segment.
     */
    if ((shmid = shmget(key, KILOBYTE, IPC_CREAT | 0666)) < 0) {
        perror("shmeget failed");
        exit(1);
    }

    /*
     * Now we attach the segment to our data space.
     */
    if ((shm = shmat(shmid, NULL, 0)) == (char *) -1) {
        perror("shmat failed");
        exit(1);
    }
    memset(shm, 0, KILOBYTE);
}

/**
 *The function releases the fifo allocated sources.
 * @param pInt file descriptor of the fifo.
 */
void ReleaseFifo(int *pInt) {
    if (close(*pInt) < 0) {
        perror("Failed closing fifo");
    }
    if (unlink(fifoID) < 0) {
        perror("Failed to unlink FIFO");
    }
}

/**
 * The function reads a process id number from the fifo.
 * @param fd fifo file descriptor.
 * @param pInt pointer to pid_t player process.
 */
void ReadPlayerPid(pid_t *pInt, int fd) {
    int bytes_read = -1;
    while(0 < sizeof(pid_t)) {
      bytes_read = read(fd, pInt, sizeof(pid_t));
    }
}

/**
 * The function generates a fifo with 0666 permissions.
 *
 */
void GenerateFifo() {
    if (mkfifo(fifoID, 0666) < 0) {
        perror("failed to mkfifo");
    }
}

/**
 * The function creates a key if sys call failed safe exits.
 * @param pInt pointer to a key.
 * @param string name of fifo
 * @param i the permissions
 */
void CreateKey(key_t *pInt, char string[7], int i) {
    if (((*pInt) = ftok(string, i)) < 0) {
        perror("Failed at ftok");
        exit(1);
    }
}