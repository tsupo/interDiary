/*
 *  interDiary.c
 *      livedoor Blog �� FC2NETWORK �ɘA���L�𓊍e����v���O����
 *
 *      Copyright (c) 2005 by H.Tsujimura
 *      written by H.Tsujimura
 *      mailto: tsupo@na.rim.or.jp
 *
 * History:
 * $Log: /comm/interDiary/interDiary.c $
 * 
 * 1     09/05/14 3:52 tsupo
 * (1) �r���h���̃f�B���N�g���\���𐮗�
 * (2) VSS�T�[�o���_��ύX
 * 
 * 12    07/12/20 15:22 Tsujimura543
 * Visual Studio 2005 �ɂ��R���p�C���`�F�b�N���ʂ𔽉f
 * 
 * 11    06/03/15 22:03 Tsujimura543
 * livedoor Blog �̃G���h�|�C���g URL �� ATOMURL_LIVEDOOR_200507 �ɌŒ�
 * 
 * 10    05/08/17 15:18 Tsujimura543
 * postEntryOnAtomAPI() �� editEntryOnAtomAPI() �̈����ǉ��ɑΉ�
 * 
 * 9     05/04/20 17:23 Tsujimura543
 * ���e���ɕt��������������C��
 * 
 * 8     05/03/29 16:36 Tsujimura543
 * �g�p����G�f�B�^���A�R�}���h���C���Ŏw��ł���悤�ɂ���
 * (notepad �ȊO�̃G�f�B�^���g����悤�ɂ���)
 * 
 * 7     05/03/29 15:25 Tsujimura543
 * Windows 98 ����(Windows 95 �ł�?) system() �ŋN������ notepad �̏I����
 * �҂����� system() ���烊�^�[�����Ă��܂����߁Aspawnlp() �� P_WAIT ��
 * notepad ���N������悤�ɕύX���Ă݂�
 * 
 * 6     05/03/28 21:11 Tsujimura543
 * livedoor Blog �ɓ��e������e������Ɍ�������
 * 
 * 5     05/03/28 20:42 Tsujimura543
 * livedoor Blog �ɓ��e������e�ɕt�����镶�������������
 * 
 * 4     05/03/28 20:36 Tsujimura543
 * FC2NETWORK�����Ɏg���^�O���������Ă݂�
 * 
 * 3     05/03/28 20:34 Tsujimura543
 * livedoor Blog �� FC2NETWORK �̗����̃A�J�E���g�������Ă���l�Ȃ�
 * �N�ł��A���L��������悤�ɂ���
 * 
 * 2     05/03/28 18:41 Tsujimura543
 * ����m�F���� (�܂����ǂ̗]�n�͂��邯�ǁA�������� fix)
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
loginFC2NETWORK( const char *username,  // (I) ���[���A�h���X
                 const char *password,  // (I) �p�X���[�h
                 char       *cookie );  // (O) �N�b�L�[

int
postFC2NETWORK( const char *username,       // (I) ���[���A�h���X
                const char *password,       // (I) �p�X���[�h
                char       *cookie,         // (I/O) �N�b�L�[
                const char *title,          // (I) �L���薼
                const char *body,           // (I) �L���{��
                const char *trackbackURL ); // (I) �g���b�N�o�b�N���M��URL


/*
 *  blog ID �̎擾�E�ݒ�
 */
#define MAX_BLOGS       10  /* ����V�X�e�����ő�ݒu�\blog�� */
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

        /* --- ���e�\�� blog �ꗗ�̎擾 */
        r = getBlogIDsOnAtomAPI( NULL, NULL, &numOfBlogs, blogInfo );
        if ( r == 1 )
            strcpy( (char *)blogID, blogInfo[0].blogID );/* �m�� */
        else if ( r > 1 ) {
            /* ---- ���e�� blog �̑I�� */
            do {
                for ( i = 0; i < r; i++ ) {
                    printf( "(%d)\t%s (blogID: %s)\n",
                            i + 1,
                            utf2sjis( blogInfo[i].blogName ),
                            blogInfo[i].blogID );
                }
                sprintf( prompt, "�ǂ� blog �ɓ��e���܂���? (1 - %d): ", r );
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


/* �L���쐬 */
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

    strcpy( title, "�A���L" );
    strcpy( contentLivedoor,
            "<p>���C�u�h�A�œ��L�������Ă݂�e�X�g�B</p>"
            "<p style=\"text-align: right;\">(�Â�)</p>"
            "<hr /><p>* ������ FC2NETWORK �̎��̓��L��������������</p>");
    strcpy( contentFC2NETWORK,
            "(�Â�)\n"
            "FC2NETWORK�œ��L�̑����������Ă݂�e�X�g�B" );

    sprintf( buf,
             "������ �薼 ������\n"
             "\n"
             "������ �薼 ������\n"
             "������ livedoor Blog �ɓ��e������e ������\n"
             "\n"
             "������ livedoor Blog �ɓ��e������e ������\n"
             "������ FC2NETWORK �ɓ��e������e ������\n"
             "\n"
             "������ FC2NETWORK �ɓ��e������e ������\n" );
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
                                          /* �K�X�A���̃G�f�B�^�ɕύX�̂��� */
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
                if ( !strncmp( p, "������ ", 7 ) ) {
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
                    if ( !strncmp( p, "������ ", 7 ) ) {
                        cnt++;
                        continue;
                    }
                    strcat( contentLivedoor, p );
                }

                if ( cnt == 5 ) {
                    if ( !strncmp( p, "������ ", 7 ) ) {
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


/* �A���L */
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
        /* livedoor Blog �� �L�� �𓊍e���� */
        CATLISTINFEX    catEx;
        char            blogID[80];
        char            *content, *subject;
        int             bRet;
        char            summary[2048];

        blogID[0] = NUL;
        setTargetURL( ATOMURL_LIVEDOOR_200507 );
        setBlogID( blogID );

        catEx.numberOfCategories = 1;
        strcpy( catEx.categoryName[0], sjis2utf( "�A���L" ) );
        strcpy( summary,
                sjis2utf( "FC2NETWORK �ƘA�g�������L���������ł��B" ) );

        subject = (char *)malloc( strlen( title ) + BUFSIZ + 1 );
        content = (char *)malloc( strlen(livedoorContent) * 3 + BUFSIZ + 1 );

        strcpy( subject, sjis2utf( title ) );

        if ( !strncmp( livedoorContent, "<p>", 3 ) )
            strcpy( content, sjis2utf( livedoorContent ) );
        else
            sprintf( content, "<p>%s</p>\n", sjis2utf( livedoorContent ) );
        strcat( content,
            sjis2utf( "<p style=\"text-align: right;\">(�Â�)</p>\n"
                      "<hr />\n"
               "<p>* ������ FC2NETWORK �̎��̓��L��������������</p>\n" ) );

        bRet = postEntryOnAtomAPI( NULL, NULL, blogID, 
                                   subject,  /* �薼           */
                                   summary,  /* �T�v           */
                                   content,  /* �{��           */
                                   &catEx,   /* �J�e�S��       */
                                   TRUE,     /* ���e���       */
                                   postID    /* ���e��̋L��ID */
                                 );

        free( subject );
        free( content );
    }

    if ( ret ) {
        /* FC2NETWORK �� ���L �𓊍e���� */
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
                     "* livedoor Blog �œ��L�̑O���AFC2NETWORK �œ��L�̌㔼"
                     "�������Ă��܂�\n (���̓��L�� livedoor Blog �ŏ������L"
                     "���ƃZ�b�g�ɂȂ���1�̓��L�A�܂�u�A���L�v���\����"
                     "�Ă��܂��B)\n\n"
                     "livedoor Blog �ŏ��������L\n"
                     "�w[:strong-s:]%s[:strong-e:]�x\n[:i-s:](%s)[:i-e:]\n"
                     "�̑����ł��B\n\n[:strong-s:](�Â�)[:strong-e:]\n%s\n",
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

