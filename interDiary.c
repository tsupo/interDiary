/*
 *  interDiary.c
 *      livedoor Blog と FC2NETWORK に連日記を投稿するプログラム
 *
 *      Copyright (c) 2005 by H.Tsujimura
 *      written by H.Tsujimura
 *      mailto: tsupo@na.rim.or.jp
 *
 * History:
 * $Log: /comm/interDiary/interDiary.c $
 * 
 * 1     09/05/14 3:52 tsupo
 * (1) ビルド環境のディレクトリ構造を整理
 * (2) VSSサーバ拠点を変更
 * 
 * 12    07/12/20 15:22 Tsujimura543
 * Visual Studio 2005 によるコンパイルチェック結果を反映
 * 
 * 11    06/03/15 22:03 Tsujimura543
 * livedoor Blog のエンドポイント URL を ATOMURL_LIVEDOOR_200507 に固定
 * 
 * 10    05/08/17 15:18 Tsujimura543
 * postEntryOnAtomAPI() と editEntryOnAtomAPI() の引数追加に対応
 * 
 * 9     05/04/20 17:23 Tsujimura543
 * 投稿時に付加する説明文を修正
 * 
 * 8     05/03/29 16:36 Tsujimura543
 * 使用するエディタを、コマンドラインで指定できるようにした
 * (notepad 以外のエディタを使えるようにした)
 * 
 * 7     05/03/29 15:25 Tsujimura543
 * Windows 98 だと(Windows 95 でも?) system() で起動した notepad の終了を
 * 待たずに system() からリターンしてしまうため、spawnlp() の P_WAIT で
 * notepad を起動するように変更してみた
 * 
 * 6     05/03/28 21:11 Tsujimura543
 * livedoor Blog に投稿する内容をさらに見直した
 * 
 * 5     05/03/28 20:42 Tsujimura543
 * livedoor Blog に投稿する内容に付加する文字列を見直した
 * 
 * 4     05/03/28 20:36 Tsujimura543
 * FC2NETWORK向けに使うタグを見直してみた
 * 
 * 3     05/03/28 20:34 Tsujimura543
 * livedoor Blog と FC2NETWORK の両方のアカウントを持っている人なら
 * 誰でも連日記を書けるようにした
 * 
 * 2     05/03/28 18:41 Tsujimura543
 * 動作確認完了 (まだ改良の余地はあるけど、いったん fix)
 * 
 * 1     05/03/28 17:14 Tsujimura543
 */

#include "xmlRPC.h"
#include "atomAPI.h"
#include <process.h>

#ifndef	lint
static char	*rcs_id =
"$Header: /comm/interDiary/interDiary.c 1     09/05/14 3:52 tsupo $";
#endif

int
loginFC2NETWORK( const char *username,  // (I) メールアドレス
                 const char *password,  // (I) パスワード
                 char       *cookie );  // (O) クッキー

int
postFC2NETWORK( const char *username,       // (I) メールアドレス
                const char *password,       // (I) パスワード
                char       *cookie,         // (I/O) クッキー
                const char *title,          // (I) 記事題名
                const char *body,           // (I) 記事本文
                const char *trackbackURL ); // (I) トラックバック送信先URL


/*
 *  blog ID の取得・設定
 */
#define MAX_BLOGS       10  /* 同一システム内最大設置可能blog数 */
#define MAX_PROMPT_LEN  80

void
setBlogID( char *blogID )
{
    if ( !(blogID[0]) ) {
        BLOGINF blogInfo[MAX_BLOGS];
        char    prompt[MAX_PROMPT_LEN];
        char    buf[BUFSIZ];
        int     numOfBlogs = MAX_BLOGS;
        int     r, i;

        /* --- 投稿可能な blog 一覧の取得 */
        r = getBlogIDsOnAtomAPI( NULL, NULL, &numOfBlogs, blogInfo );
        if ( r == 1 )
            strcpy( (char *)blogID, blogInfo[0].blogID );/* 確定 */
        else if ( r > 1 ) {
            /* ---- 投稿先 blog の選択 */
            do {
                for ( i = 0; i < r; i++ ) {
                    printf( "(%d)\t%s (blogID: %s)\n",
                            i + 1,
                            utf2sjis( blogInfo[i].blogName ),
                            blogInfo[i].blogID );
                }
                sprintf( prompt, "どの blog に投稿しますか? (1 - %d): ", r );
                inputString( (char *)buf, prompt, FALSE );
                i = atoi( buf );
                if ( (i >= 1) && (i <= r) ) {
                    strcpy( blogID, blogInfo[i - 1].blogID );
                    break;
                }
            } while ( (i < 1) || (i > r) );
        }
    }
#ifdef  _DEBUG
    fprintf( stderr, "blogID = %s\n", blogID );
#endif
}


/* 記事作成 */
int
writeDiary( char        *title,
            char        *contentLivedoor,
            char        *contentFC2NETWORK,
            const char  *editorPath )
{
#ifndef  WIN32
    char        cmd[BUFSIZ];
#endif
    char        buf[10240];
    FILE        *fp;
    char        *p;
    const char  *editorName;
    int         ret = 0;

    strcpy( title, "連日記" );
    strcpy( contentLivedoor,
            "<p>ライブドアで日記を書いてみるテスト。</p>"
            "<p style=\"text-align: right;\">(つづく)</p>"
            "<hr /><p>* 続きは FC2NETWORK の私の日記をご覧ください</p>");
    strcpy( contentFC2NETWORK,
            "(つづき)\n"
            "FC2NETWORKで日記の続きを書いてみるテスト。" );

    sprintf( buf,
             "↓↓↓ 題名 ↓↓↓\n"
             "\n"
             "↑↑↑ 題名 ↑↑↑\n"
             "↓↓↓ livedoor Blog に投稿する内容 ↓↓↓\n"
             "\n"
             "↑↑↑ livedoor Blog に投稿する内容 ↑↑↑\n"
             "↓↓↓ FC2NETWORK に投稿する内容 ↓↓↓\n"
             "\n"
             "↑↑↑ FC2NETWORK に投稿する内容 ↑↑↑\n" );
    fp = fopen( "tempEdit.txt", "w" );
    if ( fp ) {
        fputs( buf, fp );
        fclose( fp );

#ifndef WIN32
        sprintf( cmd, "%s tempEdit.txt",
                 editorPath && *editorPath ? editorPath
# ifdef  UNIX
                                           : "vi"
# else  /* !UNIX */
                                           : "notepad"
                                          /* 適宜、他のエディタに変更のこと */
# endif /* !UNIX */
               );
        system( cmd );
#else   /* WIN32 */
        editorName = editorPath && *editorPath ? editorPath : "notepad";
        spawnlp( P_WAIT, editorName, editorName, "tempEdit.txt", NULL );
#endif  /* WIN32 */

        fp = fopen( "tempEdit.txt", "r" );
        if ( fp ) {
            int cnt = 0;

            while ( ( p = fgets( buf, BUFSIZ - 1, fp ) ) != NULL ) {
                if ( !strncmp( p, "↓↓↓ ", 7 ) ) {
                    if ( cnt == 2 )
                        contentLivedoor[0]   = NUL;
                    if ( cnt == 4 )
                        contentFC2NETWORK[0] = NUL;
                    cnt++;
                    continue;
                }

                if ( cnt == 1 ) {
                    if ( p[strlen(p) - 1] == '\n' )
                        p[strlen(p) - 1] = NUL;
                    strcpy( title, p );
                    cnt++;
                    continue;
                }

                if ( cnt == 3 ) {
                    if ( !strncmp( p, "↑↑↑ ", 7 ) ) {
                        cnt++;
                        continue;
                    }
                    strcat( contentLivedoor, p );
                }

                if ( cnt == 5 ) {
                    if ( !strncmp( p, "↑↑↑ ", 7 ) ) {
                        cnt++;
                        break;
                    }
                    strcat( contentFC2NETWORK, p );
                }
            }
            fclose( fp );

            if ( cnt == 6 )
                if ( title[0] && contentLivedoor[0] && contentFC2NETWORK[0] )
                    ret = 1;
        }
    }

    return ( ret );
}


#define MAX_TITLE_LEN   512
#define MAX_BODY_LEN    (MAX_CONTENT_SIZE * 2)
#define MAX_POSTID_LEN  MAX_POSTIDLENGTH


/* 連日記 */
int
interDiary( const char *usernameLivedoor,
            const char *usernameFC2N,
            const char *passwordFC2N,
            const char *editorPath )
{
    char    postID[MAX_POSTIDLENGTH + 1];
    char    title[80];
    char    livedoorContent[20480];
    char    fc2networkContent[20480];
    int     ret;

    ret = writeDiary( title, livedoorContent, fc2networkContent, editorPath );

    if ( ret ) {
        /* livedoor Blog に 記事 を投稿する */
        CATLISTINFEX    catEx;
        char            blogID[80];
        char            *content, *subject;
        int             bRet;
        char            summary[2048];

        blogID[0] = NUL;
        setTargetURL( ATOMURL_LIVEDOOR_200507 );
        setBlogID( blogID );

        catEx.numberOfCategories = 1;
        strcpy( catEx.categoryName[0], sjis2utf( "連日記" ) );
        strcpy( summary,
                sjis2utf( "FC2NETWORK と連携した日記を書く企画です。" ) );

        subject = (char *)malloc( strlen( title ) + BUFSIZ + 1 );
        content = (char *)malloc( strlen(livedoorContent) * 3 + BUFSIZ + 1 );

        strcpy( subject, sjis2utf( title ) );

        if ( !strncmp( livedoorContent, "<p>", 3 ) )
            strcpy( content, sjis2utf( livedoorContent ) );
        else
            sprintf( content, "<p>%s</p>\n", sjis2utf( livedoorContent ) );
        strcat( content,
            sjis2utf( "<p style=\"text-align: right;\">(つづく)</p>\n"
                      "<hr />\n"
               "<p>* 続きは FC2NETWORK の私の日記をご覧ください</p>\n" ) );

        bRet = postEntryOnAtomAPI( NULL, NULL, blogID, 
                                   subject,  /* 題名           */
                                   summary,  /* 概要           */
                                   content,  /* 本文           */
                                   &catEx,   /* カテゴリ       */
                                   TRUE,     /* 投稿状態       */
                                   postID    /* 投稿後の記事ID */
                                 );

        free( subject );
        free( content );
    }

    if ( ret ) {
        /* FC2NETWORK に 日記 を投稿する */
        char    cookie[BUFSIZ * 4];
        char    livedoorURL[MAX_URLLENGTH];
        char    trackbackURL[512];
        char    *content, *subject;
        char    body[20480];

        setUserInfo( usernameFC2N, passwordFC2N );
        setBlogKind( BLOGKIND_FC2NETWORK );
        memset( cookie,  0x00, BUFSIZ * 4 );
        if ( loginFC2NETWORK( NULL, NULL, cookie ) ) {
            sprintf( trackbackURL,
                     "http://blog.livedoor.jp/%s/tb.cgi/%s",
                     usernameLivedoor,
                     postID );
            sprintf( livedoorURL,
                     "http://blog.livedoor.jp/%s/archives/%s.html",
                     usernameLivedoor,
                     postID );

            sprintf( body,
                     "* livedoor Blog で日記の前半、FC2NETWORK で日記の後半"
                     "を書いています\n (この日記は livedoor Blog で書いた記"
                     "事とセットになって1つの日記、つまり「連日記」を構成し"
                     "ています。)\n\n"
                     "livedoor Blog で書いた日記\n"
                     "『[:strong-s:]%s[:strong-e:]』\n[:i-s:](%s)[:i-e:]\n"
                     "の続きです。\n\n[:strong-s:](つづき)[:strong-e:]\n%s\n",
                     title, livedoorURL, fc2networkContent );

            subject = (char *)malloc( strlen( title ) + BUFSIZ + 1 );
            content = (char *)malloc( strlen( body ) * 3 + BUFSIZ + 1 );

            strcpy( subject, sjis2euc( title ) );
            strcpy( content, sjis2euc( body ) );

            postFC2NETWORK( NULL, NULL, cookie,
                            subject, content, trackbackURL );

            free( subject );
            free( content );
        }
    }

    return ( ret );
}



int
main( int argc, char *argv[] )
{
    int     verbose    = 0;
    int     useProxy   = 0;
    int     i, j;
    char    usernameLivedoor[80];
    char    passwordLivedoor[80];
    char    usernameFC2N[80];
    char    passwordFC2N[80];
    char    editorPath[MAX_PATH];
    int     ret = 0;
    char    *p;
    FILE    *fp;

    usernameLivedoor[0] = NUL;
    passwordLivedoor[0] = NUL;
    usernameFC2N[0]     = NUL;
    passwordFC2N[0]     = NUL;
    editorPath[0]       = NUL;

    for ( i = 1; i < argc; i++ ) {
        if ( argv[i][0] != '-' )
            break;
        for ( j = 1; argv[i][j]; j++ ) {
            switch ( argv[i][j] ) {
            case 'v':
                verbose = !verbose;
                break;

            case 'p':
                useProxy = !useProxy;
                break;

            case 'e':
                if ( argv[i][j + 1] )
                    strcpy( editorPath, &(argv[i][j + 1]) );
                else if ( i + 1 < argc )
                    strcpy( editorPath, argv[++i] );
                else
                    continue;
                j = strlen( argv[i] ) - 1;
                break;
            }
        }
    }

    fp = fopen( "setting.inf", "r" );
    if ( fp ) {
        char    buf[BUFSIZ], *q;
        int     cnt = 0;
        while ( ( p = fgets( buf, BUFSIZ - 1, fp ) ) != NULL ) {
            while ( (*p == ' ') || (*p == '\t') )
                p++;
            if ( (*p == '#') || (*p == '\n') || (*p == '\r') )
                continue;
            q = p + strlen( p );
            while ( --q > p ) {
                if ( (*q == '\n') || (*q == '\r') ||
                     (*q == ' ')  || (*q == '\t')    )
                    *q = NUL;
                else
                    break;
            }
            if ( !(*p) || (p == q) )
                continue;

            switch ( cnt ) {
            case 0:
                strcpy( usernameLivedoor, p );
                break;
            case 1:
                strcpy( passwordLivedoor, p );
                break;
            case 2:
                strcpy( usernameFC2N, p );
                break;
            case 3:
                strcpy( passwordFC2N, p );
                break;
            }

            if ( ++cnt >= 4 )
                break;
        }
        fclose( fp );
    }

    /* livedoor Blog */
    if ( usernameLivedoor[0] == NUL ) {
        do {
            fputs( "[livedoor Blog] Username: ", stderr );
            p = fgets( usernameLivedoor, 79, stdin );
            if ( !p ) {
                clearerr( stdin );
                usernameLivedoor[0] = '\0';
            }
            while ( usernameLivedoor[strlen(usernameLivedoor) - 1] == '\n' ) {
                usernameLivedoor[strlen(usernameLivedoor) - 1] = '\0';
                if ( usernameLivedoor[0] == '\0' )
                    break;
            }
        } while ( usernameLivedoor[0] == '\0' );
    }

    if ( passwordLivedoor[0] == NUL ) {
        do {
            fputs( "[livedoor Blog] Password: ", stderr );
            p = fgets( passwordLivedoor, 79, stdin );
            if ( !p ) {
                clearerr( stdin );
                passwordLivedoor[0] = '\0';
            }
            while ( passwordLivedoor[strlen(passwordLivedoor) - 1] == '\n' ) {
                passwordLivedoor[strlen(passwordLivedoor) - 1] = '\0';
                if ( passwordLivedoor[0] == '\0' )
                    break;
            }
        } while ( passwordLivedoor[0] == '\0' );
    }

    /* FC2NETWORK */
    if ( usernameFC2N[0] == NUL ) {
        do {
            fputs( "[FC2NETWORK] Username: ", stderr );
            p = fgets( usernameFC2N, 79, stdin );
            if ( !p ) {
                clearerr( stdin );
                usernameFC2N[0] = '\0';
            }
            while ( usernameFC2N[strlen(usernameFC2N) - 1] == '\n' ) {
                usernameFC2N[strlen(usernameFC2N) - 1] = '\0';
                if ( usernameFC2N[0] == '\0' )
                    break;
            }
        } while ( usernameFC2N[0] == '\0' );
    }

    if ( passwordFC2N[0] == NUL ) {
        do {
            fputs( "[FC2NETWORK] Password: ", stderr );
            p = fgets( passwordFC2N, 79, stdin );
            if ( !p ) {
                clearerr( stdin );
                passwordFC2N[0] = '\0';
            }
            while ( passwordFC2N[strlen(passwordFC2N) - 1] == '\n' ) {
                passwordFC2N[strlen(passwordFC2N) - 1] = '\0';
                if ( passwordFC2N[0] == '\0' )
                    break;
            }
        } while ( passwordFC2N[0] == '\0' );
    }

    setUserInfo( usernameLivedoor, passwordLivedoor );
    setVerbose( verbose, stderr );
    setUseProxy( useProxy );

    ret = interDiary( usernameLivedoor, usernameFC2N, passwordFC2N,
                      editorPath );

    return ( ret );
}

