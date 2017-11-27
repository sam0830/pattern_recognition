/**************************************************/
/*****            make_prtimg2.c                ***/
/*****         プリント用画像作成               ***/
/*****    Usage: a.out                          ***/
/*****    Function: 2種類の平均画像を作成       ***/
/*****    Input:  正規化画像1,正規化画像2       ***/
/*****    Output: 各文字の平均画像              ***/
/*****                                          ***/
/**************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char UCHAR;

#define MY_ID "bp13033"		/* 自分の学籍番号 */
#define AITE_ID "bp13035"	/* 相手の学籍番号 */
#define GYOU_MOJI 100		/* 切り出す画像の行数 */
#define RETU_MOJI GYOU_MOJI	/* 切り出す画像の列数 */  
#define GYOU_PRT (GYOU_MOJI*4)	/* プリント画像の行数 */
#define RETU_PRT (RETU_MOJI*5)	/* プリント画像の列数 */  
#define N_MOJI 10		/* 文字種の総数 */
#define N_SAMPLE 6		/* 1文字あたりのデータ数 */
#define PATH_FOR_DATA "/mnt/oshare/2017/大宮月曜２限パターン認識 - 2311101030/配布用" /* データのパス */

void read_cut_pgm(UCHAR [][RETU_MOJI], char *,int *,int *);
void write_pgm(UCHAR **,char *,int ,int );
void error1(char *);


int main(int argc,char *argv[])
{
  int i,j;
  int g,r;
  int g_start,r_start;		/* 書き込む領域の原点(左上座標) */
  int in_retu,in_gyou;
  UCHAR org[GYOU_MOJI][RETU_MOJI]; /* 入力画像 */
  UCHAR res[GYOU_PRT][RETU_PRT]; /* 出力画像 */
  int sum[GYOU_MOJI][RETU_MOJI]; /* 平均算出用画像 */
  char in_fname[200];		/* 入力画像のファイル名 */
  char out_fname[200];		/* 出力画像のファイル名 */


  for(i=0;i<N_MOJI;i++)		/* 前処理画像について処理 */
  {
    for(g=0;g<GYOU_MOJI;g++)	/* 加算値初期化 */
      for(r=0;r<RETU_MOJI;r++)
	sum[g][r]=0;

    for(j=0;j<N_SAMPLE;j++)
    {
      sprintf(in_fname,"%s/Mae/%s/SIGMA0/%smae-%1d-%1d.pgm",PATH_FOR_DATA,MY_ID,MY_ID,i,j); /* 入力画像ファイル名 */
      read_cut_pgm(org, in_fname, &in_retu, &in_gyou);  /* 入力画像読み込み */
      if((in_retu != RETU_MOJI) || (in_gyou != GYOU_MOJI))
	error1("Dimensions are wrong.");

      for(g=0;g<GYOU_MOJI;g++)	/* 平均値算出のため加算 */
	for(r=0;r<RETU_MOJI;r++)
	  sum[g][r] += org[g][r];
    }

    g_start=(i/5)*GYOU_MOJI*2; /* 書き込む領域の原点(左上座標)算出 */
    r_start=(i%5)*RETU_MOJI;
    for(g=1;g<GYOU_MOJI-1;g++)	/* 平均値を出力画像に書き込む(外周部は枠として黒くする) */
      for(r=1;r<RETU_MOJI-1;r++)
	res[g+g_start][r+r_start]=(UCHAR)((float)sum[g][r]/(float)(N_SAMPLE)+0.49999);
  }
  

  for(i=0;i<N_MOJI;i++)		/* 正規化画像について処理 */
  {
    for(g=0;g<GYOU_MOJI;g++)	/* 加算値初期化 */
      for(r=0;r<RETU_MOJI;r++)
	sum[g][r]=0;

    for(j=0;j<N_SAMPLE;j++)
    {
      sprintf(in_fname,"%s/Mae/%s/SIGMA0/%smae-%1d-%1d.pgm",PATH_FOR_DATA,AITE_ID,AITE_ID,i,j); /* 入力画像ファイル名 */
      read_cut_pgm(org, in_fname, &in_retu, &in_gyou);  /* 入力画像読み込み */
      if((in_retu != RETU_MOJI) || (in_gyou != GYOU_MOJI))
	error1("Dimensions are wrong.");

      for(g=0;g<GYOU_MOJI;g++)	/* 平均値算出のため加算 */
	for(r=0;r<RETU_MOJI;r++)
	  sum[g][r] += org[g][r];
    }

    g_start=(i/5)*GYOU_MOJI*2+GYOU_MOJI; /* 書き込む領域の原点(左上座標)算出 */
    r_start=(i%5)*RETU_MOJI;
    for(g=1;g<GYOU_MOJI-1;g++)	/* 平均値を出力画像に書き込む(外周部は枠として黒くする) */
      for(r=1;r<RETU_MOJI-1;r++)
	res[g+g_start][r+r_start]=(UCHAR)((float)sum[g][r]/(float)(N_SAMPLE)+0.49999);
  }
  

  sprintf(out_fname,"./%s-%s-hikaku.pgm",MY_ID,AITE_ID); /* 出力画像ファイル名 */
  write_pgm((UCHAR **)res, out_fname, RETU_PRT, GYOU_PRT);  /* 出力画像書き込み */

  return 0;
}


/* pgmフォーマットの画像ファイル作成 */
void write_pgm(UCHAR **data_buf,char *fname,int width,int height)
{
  FILE *fp;
  int i,m,n;
  UCHAR *val;

  if((val = (UCHAR *)malloc(width*height*sizeof(UCHAR))) == NULL)
    error1("Memory allocation failed in write_pgm.");

  if((fp = fopen(fname, "wb")) == NULL) {
    fprintf(stderr, "file(%s) can't open\n", fname) ;
    exit(1) ;
  }

  fprintf(fp, "P5\n") ;			/* カラー画像かつバイナリーデータの記号 */
  //  fprintf(fp, "#\t%s\n", fname) ;	/* #で始まるのはコメント行 */
  fprintf(fp, "%d %d\n", width, height) ; /* 画像の幅(列数)と高さ(行数) */
  fprintf(fp, "255\n") ;		/* 最大値 */

  i=0;
  for(m=0;m<height;m++)
    for(n=0;n<width;n++)
    {
      val[i++]=*((UCHAR *)data_buf+(m*width+n));
    }

  fwrite(val, sizeof(UCHAR), width*height, fp); /* ファイルへの書き込み */

  fclose(fp);

  free(val);
}


/* pgmフォーマットの画像ファイル読み込み */
void read_cut_pgm(UCHAR data_buf[][RETU_MOJI], char *fname,int *width,int *height)
{
  FILE	*fp ;
  char	str_buf[128] ;
  char	magic_num[8] ; /* マジックナンバー */
  int	max_val ;      /* 画素値の最大値 */    
  int	c, i, m, n ;
  int	inc = '0' ;
  int	flg = 0 ;
  int	com_flg = 0 ;
  int	break_flg = 0 ;
  long  k;
  UCHAR val[GYOU_MOJI*RETU_MOJI]; /* データ読み込み用バッファ(画像と同じサイズ) */

  /* ファイルを開く */    
  if((fp = fopen(fname, "rb")) == NULL) {
    fprintf(stderr, "file(%s) can't open.\n", fname) ;
    exit(-1) ;
  }

  /* ヘッダー部分読み込み(行数,列数設定) */  
  i = 0 ;
  while(1) 
  {
    c = getc(fp) ;

    if(com_flg) {
      if(c == '\n')
	com_flg-- ;
    }
    else {
      if(c == '#') {
	com_flg++ ;
	continue ;
      }

      if(!flg) {
	if(((c == ' ') || (c == '\t') || (c == '\n')) && (i > 0))
	  flg++ ;
	else if(((c == ' ') || (c == '\t') || (c == '\n')) && (i == 0))
	   ;
	else
	  str_buf[i++] = (char)c ;
      }

      if(flg) {
	str_buf[i] = '\0' ;
	i = 0 ;
	  
	switch(inc) {
	case '0':
	  strcpy(magic_num, str_buf) ;
	  if(strcmp(magic_num, "P5") != 0) {
	    fprintf(stderr, "read_ppm: ERROR: magic number(%s) not match.\n", magic_num) ;
	    exit(-1) ;
	  }
	  inc++ ;
	  break ;
	case '1':
	  *width = atoi(str_buf) ;
	  inc++ ;
	  break ;
	case '2':
	  *height = atoi(str_buf) ;
	  inc++ ;
	  break ;
	case '3':
	  max_val = atoi(str_buf) ;
	  break_flg++ ;
	  break ;
	}
	flg-- ;
      }
    }
    if(break_flg)
      break ;
  }

  /* 画像のサイズが仮定した値(GYOUxRETU)と異なる場合はエラーで終了 */
  if((*height != GYOU_MOJI) || (*width != RETU_MOJI))
  {
    fprintf(stderr, "read_ppm: ERROR: Dimension dosenot match.\n");
    exit(-1) ;
  }

  /* 画像データ部分読み込み */
  k=0;
  fread(val, sizeof(UCHAR), GYOU_MOJI*RETU_MOJI, fp);
  for( m = 0; m < *height; m++)
    for( n = 0; n < *width; n++)
    {
      data_buf[m][n] = val[k++];
    }

  /* ファイルを閉じる */    
  fclose(fp) ;
}


/* エラー処理 */
void error1(char *message)
{
  printf("%s\n",message);
  exit(1);
}
