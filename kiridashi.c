/**************************************************/
/*****             kiridashi.c                  ***/
/*****             文字の切出し                 ***/
/*****      Usage: a.out ID                     ***/
/*****   Output: ID-?-#.pgmが新たに作成される   ***/
/*****        ?: 数字                           ***/
/*****        #: 通し番号                       ***/
/**************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char UCHAR;

#define GYOU 1400		/* 原画像の行数 */
#define RETU 910		/* 原画像の列数 */
#define GYOU_MOJI 100		/* 切り出す画像の行数 */
#define RETU_MOJI GYOU_MOJI	/* 切り出す画像の列数 */  
#define N_MOJI 10		/* 文字種の総数 */
#define N_SAMPLE 6		/* 1文字あたりのデータ数 */
#define TH_BORDER 100		/* 画素値がこの値以下の場合に境界線とみなす */

void read_pgm(UCHAR [][RETU], char *,int *,int *);
void calc_gyou_heikin(UCHAR [][RETU],UCHAR [][RETU],int ,int );
void calc_retu_heikin(UCHAR [][RETU],UCHAR [][RETU],int ,int );
void write_pgm(UCHAR [][RETU],char *,int ,int );
void write_cut_pgm(UCHAR [][RETU_MOJI],char *,int ,int );
void error1(char *);


int main(int argc,char *argv[])
{
  UCHAR org[GYOU][RETU];	/* 入力画像 */
  UCHAR res1[GYOU][RETU];	/* 行方向平均値(1次元で良いが，表示のため画像としている) */
  UCHAR res2[GYOU][RETU];	/* 列方向平均値 */
  UCHAR cut_img[GYOU_MOJI][RETU_MOJI]; /* 切り出した文字画像(1文字分) */
  int in_retu;			/* 入力画像の列数 */
  int in_gyou;			/* 入力画像の行数 */
  char id[10];			/* ID(学籍番号) */
  char in_fname[200];		/* 入力画像のファイル名 */
  char out_fname[200];		/* 出力画像のファイル名 */
  int g,r;
  int g_moji,r_moji;
  int g_center,r_center;	/* 境界線間の中央 */
  int g_bord[N_MOJI+1];		/* 境界線座標(行) */
  int r_bord[N_SAMPLE+2];	/* 境界線座標(列) */
  int n_border;


  /***** IDをコマンドラインから代入 *****/
  if(argc == 2)
  {
    strcpy(id,argv[argc-1]);
  }
  else
  {
    printf("usage: %s ID\n",argv[0]);
    exit(1);
  }

  sprintf(in_fname,"%s.pgm",id); /* 入力画像ファイル名 */
    
  /***** 画像の読み込み *****/
  read_pgm(org, in_fname, &in_retu, &in_gyou);

  /***** 行方向平均画素値算出 *****/
  calc_gyou_heikin(res1,org,in_gyou,in_retu);
  sprintf(out_fname,"%s-gyouheikin.pgm",id); /* 出力画像ファイル名 */
  write_pgm(res1,out_fname,in_retu,in_gyou);  /***** 変換後画像をファイルへ出力 *****/

  /***** 列方向平均画素値算出 *****/
  calc_retu_heikin(res2,org,in_gyou,in_retu);
  sprintf(out_fname,"%s-retuheikin.pgm",id); /* 出力画像ファイル名 */
  write_pgm(res2,out_fname,in_retu,in_gyou);  /***** 変換後画像をファイルへ出力 *****/

#ifdef REMOVE
				/* 境界設定 */
  n_border=N_MOJI;
  for(g=GYOU-1;g>0;g--)
  {
    if(res1[g][0]<TH_BORDER)
    {
      g_bord[n_border]=g;
      n_border--;
      if(n_border < 0)		/* 最後の境界線を検出した場合 */
	g=0;			/* 境界設定処理を終わる */
      else
	g-=(GYOU_MOJI-20);	/* 次の境界線を探すためにジャンプする */
    }
  }

  n_border=N_SAMPLE+1;
  for(r=RETU-1;r>0;r--)
  {
				/* (A)この中を書く */



  }

				/* 画像切出し */
  for(g_moji=0;g_moji<N_MOJI;g_moji++)
    for(r_moji=1;r_moji<N_SAMPLE+1;r_moji++)
    {
      g_center=			/* (B)この2行も書く */
      r_center=
      for(g=0;g<GYOU_MOJI;g++)
	for(r=0;r<RETU_MOJI;r++)
	  cut_img[g][r]=org[g_center-(int)GYOU_MOJI/2+g][r_center-(int)RETU_MOJI/2+r];

      sprintf(out_fname,"Org/%s/%s-%1d-%d.pgm",id,id,g_moji,r_moji-1); /* 出力画像ファイル名 */
      write_cut_pgm(cut_img,out_fname,RETU_MOJI,GYOU_MOJI);  /***** 変換後画像をファイルへ出力 *****/
    }
#endif

  return 0;
}

/***** 行平均値算出 *****/
void calc_gyou_heikin(UCHAR res[][RETU],UCHAR org[][RETU],int gyou,int retu)
{
  int g,r;
  float sum;
  UCHAR av;

  for(g=0;g<gyou;g++)
  {
				/* この中を書く */

    for(r=0;r<retu;r++)
      res[g][r]=av;
  }
}

/***** 列平均値算出 *****/
void calc_retu_heikin(UCHAR res[][RETU],UCHAR org[][RETU],int gyou,int retu)
{
  int g,r;
  float sum;
  UCHAR av;

  for(r=0;r<retu;r++)
  {
				/* この中を書く */

    for(g=0;g<gyou;g++)
      res[g][r]=av;
  }
}


/* pgmフォーマットの画像ファイル作成 */
void write_pgm(UCHAR data_buf[][RETU],char *fname,int width,int height)
{
  FILE *fp;
  int i,m,n;
  UCHAR *val;

  if((val = (UCHAR *)malloc(width*height*sizeof(UCHAR))) == NULL)
    error1("Memory allocation failed.");

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
      val[i++]=data_buf[m][n];
    }

  fwrite(val, sizeof(UCHAR), width*height, fp); /* ファイルへの書き込み */

  fclose(fp);

  free(val);
}

/* pgmフォーマットの画像ファイル作成 */
void write_cut_pgm(UCHAR data_buf[][RETU_MOJI],char *fname,int width,int height)
{
  FILE *fp;
  int i,m,n;
  UCHAR *val;

  if((val = (UCHAR *)malloc(width*height*sizeof(UCHAR))) == NULL)
    error1("Memory allocation failed.");

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
      val[i++]=data_buf[m][n];
    }

  fwrite(val, sizeof(UCHAR), width*height, fp); /* ファイルへの書き込み */

  fclose(fp);

  free(val);
}

/* エラー処理 */
void error1(char *message)
{
  printf("%s\n",message);
  exit(1);
}


/* pgmフォーマットの画像ファイル読み込み */
void read_pgm(UCHAR data_buf[][RETU], char *fname,int *width,int *height)
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
  UCHAR val[GYOU*RETU]; /* データ読み込み用バッファ(画像と同じサイズ) */

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
  if((*height != GYOU) || (*width != RETU))
  {
    fprintf(stderr, "read_ppm: ERROR: Dimension dosenot match.\n");
    exit(-1) ;
  }

  /* 画像データ部分読み込み */
  k=0;
  fread(val, sizeof(UCHAR), GYOU*RETU, fp);
  for( m = 0; m < *height; m++)
    for( n = 0; n < *width; n++)
    {
      data_buf[m][n] = val[k++];
    }

  /* ファイルを閉じる */    
  fclose(fp) ;
}
