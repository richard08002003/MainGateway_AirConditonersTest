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

#define DF_API_UNSKT_FILE_PATH "/tmp/apiUxSkt.ipc"

FILE *fp1 , *fp2 , *fp3 , *fp4 ;
int main(void) {
    struct sockaddr_un client ;
    /**Create a socket*/
    int t = 0 ;
    for( t =0 ; t < 3 ; t++ )  {
        int sock = socket( AF_UNIX , SOCK_STREAM , 0 ) ;
        if ( sock < 0 ) {
            perror("socket") ;
            exit(1) ;
        }
        printf("Socket Created ! \n") ;

        printf("Trying to connecting . . . \n") ;
        client.sun_family = AF_UNIX ;
        strcpy( client.sun_path , DF_API_UNSKT_FILE_PATH ) ;
        /**connect() */
        int len = sizeof(client) ;
        int conn = connect( sock , (struct sockaddr *)&client , len ) ;
        if ( conn < 0 ) {
            perror("connect") ;
            exit(1) ;
        }
            printf("\nConnecting to Server ! ! ! \n") ;
        /** fork() : 寫資料＆讀資料*/
        pid_t fpid ;
        fpid = fork() ;
        if ( fpid < 0 ) {
            printf("fork is Error ! \n") ;
            exit(1) ;
        } else if ( fpid == 0 ) {
            /**child process
            **send() 寫資料*/

                    printf("開啟 %s 檔案\n" , "data1.txt") ;
                    fp1 = fopen( "data1.txt" , "r" ) ; //開啟檔案data.txt
                    fp2 = fopen( "parsedata3.txt" , "w" ) ;//開啟檔案result.txt
                    if ( (fp1 == NULL) || (fp2 == NULL) ) {
                        printf("檔案開啟錯誤！\n") ;
                        break ;
                    }
                    printf("正在讀取資料 . . . \n") ;
                    char c , d , e ;
                    unsigned char str[200]={0} ;
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
                    printf("%s分析完畢 . . . \n", "data1.txt") ;
                    fclose(fp1) ;
                    fclose(fp2) ;

                    fp3 = fopen( "parsedata3.txt" , "r" )  ; //開啟parsedata.txt
                    fp4 = fopen( "Api_SendLog.txt" , "w" ) ;
                    if ( (fp3 ==NULL) || (fp4 ==NULL) ) {
                        printf("檔案開啟錯誤！\n") ;
                        exit(1);
                    }
                    //從分析完的檔案當中抓取一行字串 , 並且傳送給MaingatewayPgm
                    while( fgets( str , sizeof(str)-1 , fp3 ) != NULL ) {
                        if ( str[strlen(str) - 1] == '\n' ) {
                            str[strlen(str) -1 ] = '\0' ;
                        }
                        printf("傳送的資料為：%s \n", str ) ;
                        fprintf( fp4 , "%s" , "傳送的資料為：" )  ;
                        fprintf( fp4 , "%s\n" , str ) ;

                        //send() 將一筆一筆資料傳給MainGateway
                        if( send( sock , str , strlen(str) , 0) < 0 ) {
                            printf("Send Failed ! \n") ;
                            break ;
                        }
                        printf("Send Success ! \n") ;
                        fprintf( fp4 , "%s" , "Send Success ! \n") ;
                        sleep(1) ;
                    }
                    fclose(fp3) ;
                    fclose(fp4) ;

        } else {
            /**parent process
            **recv() 讀取資料*/
                while(1) {
                //初始化str[ ]
                    unsigned char str[1000] = {0} ;
                    //recv() 接收來自maingateway server的信息
                    int val = recv( sock , str , sizeof(str) , 0 ) ;
                    if ( val < 0 ) {
                        printf("\nReceive messages Failed!! \n") ;
                        break ;
                    } else if ( val == 0 ) {
                        printf("\nDisconnect to Maingateway server . . . \n") ;
                        break ;
                    } else {
                        printf("\n接收 Server 的資料為 --> %s \n" , str ) ;

                        printf("連線中斷 . . . \n") ;
                        sleep(2) ;
                        break ;
                    }
                }
            }
            close(sock) ;
            t+1 ;
        }
        return 0 ;
}
