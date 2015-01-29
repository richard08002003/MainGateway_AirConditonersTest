#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <unistd.h>
#include <fcntl.h>
#include <openssl/md5.h>

#define DF_CPS_UNSKT_FILE_PATH         "/tmp/cpsUxSkt.ipc"

FILE *fp1 , *fp2 , *fp3 , *fp4  , *fp5 ;
int main(void) {
    struct sockaddr_un server , client ;
    /**Create a socket*/
    int sock = socket( AF_UNIX , SOCK_STREAM , 0 ) ;
    if ( sock < 0) {
        perror("socket") ;
        exit(1) ;
    }
    printf("Socket Created!\n") ;

    server.sun_family = AF_UNIX ;
    strcpy(server.sun_path , DF_CPS_UNSKT_FILE_PATH) ;
    unlink(server.sun_path) ;
    int len = strlen(server.sun_path) + sizeof(server.sun_family) ;
    /**bind() */
    if ( bind(sock , (struct sockaddr *)&server , len) < 0 ) {
        perror("bind") ;
        exit(1) ;
    }
        printf("bind successed!\n") ;

    /**listen() */
    if ( listen( sock , 10 ) < 0 ) {
        perror("listen") ;
        exit(1) ;
    }
        printf("listen successed!\n") ;

    while(1) {
        printf("Waiting for connection . . . \n") ;
        int clen = sizeof(client) ;
        /**accept() 接受client 端的連線要求*/
        int sock2 = accept( sock , (struct sockaddr *)&client , &clen ) ;
        if ( sock2 < 0 ) {
            perror("accept") ;
            exit(1) ;
        }
            printf("\nSuccess to connect!!\n") ;

        /** fork() : 寫資料＆讀資料*/
        pid_t fpid ;
        fpid = fork() ;
        if ( fpid < 0 ) {
            printf("fork is Error ! \n ") ;
            exit(1) ;
        } else if ( fpid == 0 ) {
            /**child process
           **send()  寫資料*/
            while(1) {
                while(1) {
                    printf("請輸入欲傳送的檔案名稱：") ;
                    char file1[100] = {0} ;
                    fgets( file1 , sizeof(file1) , stdin ) ;
                    char *p =strrchr( file1 , '\n' ) ;
                    if ( p != NULL) {
                        *p = 0 ;
                    }
                    printf("%s" , file1) ;
                    fp1 = fopen( file1 , "r" ) ; //開啟檔案data.txt
                    fp2 = fopen( "parsedata.txt" , "w" ) ;//開啟檔案result.txt
                    if ( (fp1 == NULL) || (fp2 == NULL) ) {
                        printf("檔案開啟錯誤！\n") ;
                        exit(1) ;
                    }
                    printf("正在讀取資料 . . . \n") ;
                    char c , d , e ;
                    unsigned char str[200] = {0} ;
                    //將原始資料分析
                    while( ( c = fgetc(fp1) ) != EOF ) {
                        if ( c == '/') {
                            d = fgetc(fp1) ;
                            if ( d == 'S' ) {
                                e = fgetc(fp1) ;
                                if ( e== '>' ) {
                                    fprintf( fp2 , "%c" , c ) ;
                                    fprintf( fp2 , "%c" , d ) ;
                                    fprintf( fp2 , "%c" , e ) ;
                                    fprintf( fp2 , "%c" , '\n') ;
                                }
                            }
                        } else {
                        fprintf( fp2 , "%c" , c ) ;
                        }
                    }
                    fclose(fp1) ;
                    fclose(fp2) ;
                    fp3 = fopen( "parsedata.txt" , "r" )  ; //開啟parsedata.txt
                    fp4 = fopen( "Cps_SendLog.txt" , "w" ) ;
                    if ( (fp3 ==NULL) || (fp4 ==NULL) ) {
                        printf("檔案開啟錯誤！\n") ;
                        exit(1) ;
                    }

                    //從分析完的檔案當中抓取一行字串 , 並且傳送給MaingatewayPgm
                    while( fgets( str , sizeof(str)-1 , fp3 ) != NULL ) {
                        if ( str[strlen(str) - 1] == '\n' ) {
                            str[strlen(str) -1 ] = '\0' ;
                        }
                        printf("傳送的資料為：%s \n", str ) ;
                        fprintf( fp4 , "%s" , "傳送的資料為：" )  ;
                        fprintf( fp4 , "%s\n" , str ) ;

                        /**將所得到的JSON資料訊息壓成MD5編碼*/
                        MD5_CTX ctx ;
                        //MD5_Init ()
                        int check = MD5_Init(&ctx) ;
                        if ( check == 0 ) {
                            printf("MD5_Init Failed ! \n") ;
                            fprintf( fp4 , "%s" , "MD5_Init Failed ! \n" ) ;
                            exit(1) ;
                        }
                        //MD5_Update() 將要編碼的資料放到MD5_CTX *c 內
                        MD5_Update(&ctx , str , strlen(str) ) ;
                        unsigned char md5code[16] = {0} ;
                        MD5_Final( md5code , &ctx ) ;
                        int i ;
                        printf("MD5編碼為 : ") ;
                        fprintf( fp4 , "%s" , "MD5編碼為 :" ) ;
                        for ( i = 0 ; i < 16 ; i++ ) {
                            printf( "%02X" , md5code[i] ) ;
                            fprintf( fp4 , "%02X" , md5code[i] ) ;
                        }
                        printf("\n") ;
                        fprintf( fp4 , "%s" , "\n" ) ;

                        //send() 將資料寄給MainGateway
                        if( send( sock2 , str , strlen(str) , 0) < 0 ) {
                            printf("Send Failed ! \n") ;
                            fprintf( fp4 , "%s" , "Send Failed ! \n" ) ;
                            break ;
                        }
                        printf("Send Success ! \n") ;
                        fprintf( fp4 , "%s" , "Send Success ! \n") ;
                        sleep(1) ;
                    }
                    fclose(fp3) ;
                    fclose(fp4) ;
                }
            }

        } else {
            /**parent process
            ** recv() 讀取資料*/
                while(1) {
                    //初始化str[ ]
                    unsigned char str[1000] ={0} ;
                    fp5 = fopen( "Cps_ReceiveLog.txt" , "w" ) ;
                    if ( fp5 == NULL ) {
                        printf("檔案開啟錯誤 ! \n") ;
                        exit(1) ;
                    }
                    //recv() 接收來自maingateway client 端的信息
                    int val = recv( sock2 , str , sizeof(str) , 0) ;
                    if( val < 0 ) {
                        printf("\nReceive messages Failed!! \n") ;
                        fprintf( fp5 , "%s" , "\nReceive messages Failed!! \n" ) ;
                        break ;
                    } else if ( val == 0 ) {
                        printf("\nDisconnected to Maingateway client . . . \n") ;
                        fprintf( fp5 , "%s" , "\nDisconnected to Maingateway client . . . \n" ) ;
                        break ;
                    } else {
                        printf("\n接收 Client's 的資料為：%s \n", str ) ;
                        fprintf( fp5 , "%s" , "\n接收 Client's 的資料為：" ) ;
                        fprintf( fp5 , "%s\n" , str ) ;

                        /** 將收到的資料壓成MD5 */
                        MD5_CTX ctx ;
                        int check = MD5_Init(&ctx) ;
                        if ( check == 0 ) {
                            printf("MD5_Init Failed ! \n") ;
                            fprintf( fp5 , "%s" , "MD5_Init Failed ! \n" ) ;
                            exit(1) ;
                        }
                         //MD5_Update() 將要編碼的資料放到MD5_CTX *c 內
                        MD5_Update(&ctx , str , strlen(str) ) ;
                        unsigned char md5code[16] = {0} ;
                        MD5_Final( md5code , &ctx ) ;
                        int i ;
                        printf("MD5編碼為 : ") ;
                        fprintf( fp5 , "%s" , "MD5編碼為 : " ) ;
                        for ( i = 0 ; i < 16 ; i++ ) {
                            printf("%02X" , md5code[i] ) ;
                            fprintf( fp5 , "%02X" , md5code[i] ) ;
                        }
                        printf("\n") ;
                        fprintf( fp5 , "%s" , "\n\n" ) ;
                    }
                }
        }
        close(sock2) ;
        fclose(fp5) ;
    }
    close(sock) ;
    return 0 ;
}
