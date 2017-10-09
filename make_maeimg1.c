/**************************************************/
/*****            make_maeimg1.c                ***/
/*****       前処理画像の合成画像作成           ***/
/*****    Usage: a.out ID                       ***/
/*****                                          ***/
/**************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char UCHAR;

#define GYOU_MOJI 100		/* 前処理画像の行数 */
#define RETU_MOJI GYOU_MOJI	/* 前処理画像の列数 */  
#define GYOU_PRT (GYOU_MOJI*10)	/* プリント画像の行数 */
#define RETU_PRT (RETU_MOJI*6)	/* プリント画像の列数 */  
#define N_MOJI 10		/* 文字種の総数 */
#define N_SAMPLE 6		/* 1文字あたりのデータ数 */

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
  char in_fname[200];		/* 入力画像のファイル名 */
  char out_fname[200];		/* 出力画像のファイル名 */
  char MY_ID[10];
  

  if(argc != 2)
    error1("a.out ID");
  else
  {
    strcpy(MY_ID,argv[1]);
  }

  for(i=0;i<N_MOJI;i++)
  {
    for(j=0;j<N_SAMPLE;j++)
    {
      sprintf(in_fname,"./Mae/%s/%smae-%1d-%1d.pgm",MY_ID,MY_ID,i,j); /* 入力画像ファイル名 */
      read_cut_pgm(org, in_fname, &in_retu, &in_gyou);  /* 入力画像読み込み */
      if((in_retu != RETU_MOJI) || (in_gyou != GYOU_MOJI))
	error1("Dimensions are wrong.");

      g_start=GYOU_MOJI*i;	/* 書き込む領域の原点(左上座標)算出 */
      r_start=RETU_MOJI*j;
      for(g=1;g<GYOU_MOJI-1;g++)	/* 平均値を出力画像に書き込む(外周部は枠として黒くする) */
	for(r=1;r<RETU_MOJI-1;r++)
	  res[g+g_start][r+r_start]=org[g][r];
    }
  }

  sprintf(out_fname,"./%s-mae.pgm",MY_ID); /* 出力画像ファイル名 */
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
