/*************************************************************/
/*****                    match.c                          ***/
/*****    Usage: a.out                                     ***/
/*****    Function: パターン照合による認識                 ***/
/*****    教師データ: num_sample_tch*num_id_tch文字の平均  ***/
/*****    テストデータ: n_sample_test*num_id_test文字利用  ***/
/*****    Output: 認識率                                   ***/
/*****    コンパイルオプション:                            ***/
/*****    -DSENKEI 線形識別関数を利用                      ***/
/*****    -DSOUKAN 相互相関を利用                          ***/
/*****                                                     ***/
/*************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef unsigned char UCHAR;

#define N_ID 121		/* 学習(orテスト)に使用するIDの最大数 */
#define LEN_ID 8		/* IDの文字数+1(IDは6文字か7文字) */
#define GYOU_MOJI 100		/* 切り出す画像の行数 */
#define RETU_MOJI GYOU_MOJI	/* 切り出す画像の列数 */
#define N_MOJI 10		/* 文字種の総数 */
#define AVE_VALUE 20		/* 反転画像の平均画素値をこの値にそろえる(固定) */
#define BLACK 0			/* 黒の画素値 */
#define WHITE 255		/* 白の画素値 */

#define PATH_FOR_DATA "/home/takahiro/work/pattern_recognition" /* データのパス */

//#define KYOUSHI_ID_FILENAME "./alllist.txt"  /* 教師データとして利用するIDのリスト */
//#define TEST_ID_FILENAME    "./alllist.txt" /* テストデータとして利用するIDのリスト */
#define TEST_ID_FILENAME "alllist2.txt"
// #define KYOUSHI_ID_FILENAME "./halflist1.txt"  /* 教師データとして利用するIDのリスト */
// #define TEST_ID_FILENAME    "./halflist2.txt" /* テストデータとして利用するIDのリスト */
#define KYOUSHI_ID_FILENAME "singlelist.txt"
// #define TEST_ID_FILENAME "singlelist.txt"

				/* 各ID,各文字について教師データに用いるデータ番号を指定 */
int sample_tch[]={0,1,2,3,4,5};
				/* 各ID,各文字についてテストデータに用いるデータ番号を指定 */
int sample_test[]={0,1,2,3,4,5};

int n_sample_tch=sizeof(sample_tch)/sizeof(int); /* 各ID,各文字についての教師データ数 */
int n_sample_test=sizeof(sample_test)/sizeof(int); /* 各ID,各文字についてのテストデータ数 */

int read_id_list(char [][LEN_ID],char []);
float calc_norm(UCHAR [],int);
float naiseki(UCHAR [],UCHAR [],int);
void hanten(UCHAR [][RETU_MOJI],UCHAR [][RETU_MOJI],int ,int );
void average_adjustment(UCHAR [],double ,int );
void read_pgm(UCHAR **,char *,int *,int *);
void write_pgm(UCHAR **,char *,int ,int );
void error1(char *);


int main(int argc,char *argv[])
{
  int i,j,m,n;
  int g,r;
  int in_retu,in_gyou;		/* 読み込んだ画像の列数，行数 */
  int num_id_tch;		/* 教師データ用ID(学籍番号)の総数 */
  int num_id_test;		/* テストデータ用の総数 */
  char id_tch[N_ID][LEN_ID];	/* 教師データ用ID */
  char id_test[N_ID][LEN_ID];	/* テストデータ用ID */
  UCHAR kyoushi[N_MOJI][GYOU_MOJI][RETU_MOJI]; /* 教師データ */
  UCHAR test[GYOU_MOJI][RETU_MOJI]; /* テストデータ */
  int tmp[GYOU_MOJI][RETU_MOJI]; /* 一時メモリ */
  char in_fname[200];		/* 入力画像のファイル名 */
  char out_fname[200];		/* 出力画像のファイル名 */
  int ans_number=0;		/* 算出された答え */
  float eval;			/* 評価値 */
  float eval_ans;		/* 算出された答えの評価値(距離，識別関数値など) */
  long n_moji;			/* あるIDのある文字についてのテスト回数 */
  long n_correct_moji;		/* そのうちの正解数 */
  long n_id;			/* あるIDの全文字についてのテスト回数 */
  long n_correct_id;		/* そのうちの正解数 */
  long n_total;			/* テスト用全IDの全文字についてのテスト回数 */
  long n_correct_total;		/* そのうちの正解数 */
  float rate;			/* 各IDの正解率 */
  float rate_max=0.0;		/* テストデータの各IDごとの正解率の最大値 */
  int id_rate_max=0;		/* その時のID番号 */
  float rate_min=0.0;		/* テストデータの各IDごとの正解率の最小値 */
  int id_rate_min=0;		/* その時のID番号 */
#ifdef SENKEI
  float bias[N_MOJI];		/* 線形識別関数用のバイアス値(ノルムの2乗/2) */
#endif
#ifdef SOUKAN
  float norm_kyoushi[N_MOJI];	/* 各文字の教師データのノルム(ベクトル長) */
  float norm_test;		/* テストデータのノルム */
#endif


  /***** IDの読み込み *****/
  num_id_tch=read_id_list(id_tch, KYOUSHI_ID_FILENAME);/* 教師データ用ID読み込み */
  printf("Number of ID for tch = %d\n",num_id_tch);
  num_id_test=read_id_list(id_test, TEST_ID_FILENAME);/* テストデータ用ID読み込み */
  printf("Number of ID for test = %d\n",num_id_test);

  {
    /* 教師データ作成(各数字についてnum_id_tch*n_sample_tch文字の平均を算出する) */
    for(i=0;i<N_MOJI;i++)
    {
      for(g=0;g<GYOU_MOJI;g++)	/* 加算値を0へ初期化 */
	for(r=0;r<RETU_MOJI;r++)
	  tmp[g][r]=0;

      for(n=0;n<num_id_tch;n++)
	for(j=0;j<n_sample_tch;j++)
	{			/* 教師データのファイル名 */
	  sprintf(in_fname,"%s/Mae/%s/SIGMA0/%smae-%1d-%1d.pgm",PATH_FOR_DATA,id_tch[n],id_tch[n],i,sample_tch[j]);
	  read_pgm((UCHAR **)kyoushi[i], in_fname, &in_retu, &in_gyou);	/* ベクトルとして読み込む */
	  if((in_retu != RETU_MOJI) || (in_gyou != GYOU_MOJI))
	    error1("Dimensions are wrong.");
				/* 画素値を反転する(文字が無い場合を原点とするため) */
	  hanten(kyoushi[i], kyoushi[i], in_gyou, in_retu);
				/* 反転した画像の画素値の平均をAVE_VALUEにそろえる */
	  average_adjustment((UCHAR *)kyoushi[i], AVE_VALUE, in_retu*in_gyou);

	  for(g=0;g<GYOU_MOJI;g++) /* 平均算出のために加算 */
	    for(r=0;r<RETU_MOJI;r++)
	      tmp[g][r] += kyoushi[i][g][r];
	}

      for(g=0;g<GYOU_MOJI;g++)	/* 加算回数で割って四捨五入 */
	for(r=0;r<RETU_MOJI;r++)
	  kyoushi[i][g][r]=(UCHAR)((float)tmp[g][r]/(float)n_sample_tch/(float)num_id_tch+0.5);

				/* 教師データを反転した画像をファイルとして保存(教師データ自体は反転されない) */
      hanten(test, kyoushi[i], in_gyou, in_retu);/* 画像保存にtestを使用しているが何でも良い */
      sprintf(out_fname,"./TCH/%1d-tch.pgm",i);
      write_pgm((UCHAR **)test, out_fname, in_retu, in_gyou);
    }


    /* テスト用の全IDについて評価 */
#ifdef SENKEI
    for(i=0;i<N_MOJI;i++)	/* 線形識別関数で用いるバイアス値(ノルムの2乗/2)算出 */
      bias[i]=calc_norm((UCHAR *)kyoushi[i], in_gyou*in_retu)/2.0;
#endif
#ifdef SOUKAN
    for(i=0;i<N_MOJI;i++)	/* 相互相関で用いる教師データのノルム算出 */
      norm_kyoushi[i]= calc_norm((UCHAR *)kyoushi[i], in_gyou*in_retu);		/* (A)この行を完成させる */
#endif

    rate_max=-1.0;		/* rate_max,rate_minの初期化 */
    rate_min=101.0;
    n_correct_total=0;		/* 全IDの全データでの正解数を初期化 */
    n_total=0;
    for(n=0;n<num_id_test;n++)
    {
      n_correct_id=0;		/* そのIDの全データでの正解数を初期化 */
      n_id=0;
      for(i=0;i<N_MOJI;i++)
      {
	printf("%2d  ",i);
	n_correct_moji=0;	/* 各文字の正解数を初期化 */
	n_moji=0;
	for(j=0;j<n_sample_test;j++)
	{			/* テスト画像ファイル名 */
	  sprintf(in_fname,"%s/Mae/%s/SIGMA0/%smae-%1d-%1d.pgm",PATH_FOR_DATA,id_test[n],id_test[n],i,sample_test[j]);
	  read_pgm((UCHAR **)test, in_fname, &in_retu, &in_gyou);  /* テスト画像読み込み */
	  if((in_retu != RETU_MOJI) || (in_gyou != GYOU_MOJI))
	    error1("Dimensions are wrong.");

	  hanten(test, test, in_gyou, in_retu); /* 画素値を反転する */

	  average_adjustment((UCHAR *)test, AVE_VALUE, in_retu*in_gyou); /* 平均値をAVE_VALUEに規格化 */

#ifdef SOUKAN
	  norm_test= calc_norm((UCHAR *)test, in_gyou*in_retu);		/* (B)この行を完成させる */
#endif
	  eval_ans=-GYOU_MOJI*RETU_MOJI*WHITE*WHITE-1.0; /* 評価値を考えられる最小値に設定 */
	  for(m=0;m<N_MOJI;m++)
	  {			/* 評価値算出 */
#ifdef SENKEI
	    eval=naiseki((UCHAR *)test, (UCHAR *)kyoushi[m], in_gyou*in_retu)-bias[m];
#endif
#ifdef SOUKAN
	    eval=naiseki((UCHAR *)test, (UCHAR *)kyoushi[m], in_gyou*in_retu)/sqrt(norm_test*norm_kyoushi[m]);		/* (C)この行を完成させる */
#endif
	    if(eval > eval_ans)	/* 評価値が最大となる数字mを保存 */
	    {
	      eval_ans=eval;
	      ans_number=m;
	    }
	  }
	  if(ans_number == i)	/* 正解の場合 */
	  {
	    printf("  ");
	    n_correct_moji++;
	  }
	  else			/* 不正解の場合は間違えた数字を表示 */
	  {
	    printf("%1d ",ans_number);
	  }
	  n_moji++;
	}                      /* 各IDの各文字の正解率表示 */
	printf("%ld/%ld (%5.1f%%)\n",n_correct_moji,n_moji,(float)n_correct_moji/(float)n_moji*100.0);
	n_correct_id += n_correct_moji;
	n_id += n_moji;
      }
      rate=(float)n_correct_id/(float)n_id*100.0;
      printf("ID=%s %ld/%ld (%5.1f%%)\n\n",id_test[n],n_correct_id,n_id,rate); /* 各IDでの正解率表示 */
      if(rate > rate_max)	/* 正解率最大，最小のIDを求める処理 */
      {
	rate_max=rate;
	id_rate_max=n;
      }
      if(rate < rate_min)
      {
	rate_min=rate;
	id_rate_min=n;
      }
      n_correct_total += n_correct_id;
      n_total += n_id;
    }                          /* 全データでの正解率表示 */
    printf("TOTAL %ld/%ld (%6.2f%%)\n",n_correct_total,n_total,(float)n_correct_total/(float)n_total*100.0);
    printf("Best ID  = %s (%5.1f%%)\n",id_test[id_rate_max],rate_max); /* 正解率最大，最小IDの表示 */
    printf("Worst ID = %s (%5.1f%%)\n\n",id_test[id_rate_min],rate_min);
  }
  return 0;
}

/* パターンのノルム(ベクトルの長さ)の自乗を算出 (dim:ベクトルの次元数) */
float calc_norm(UCHAR img[],int dim)
{
  int i;
  float norm=0.0;

  for(i=0;i<dim;i++) {
      /* (D) この行を完成させる */
      norm += img[i]*img[i];
  }
  return norm;
}

/* 内積を算出 (dim:ベクトルの次元数) */
float naiseki(UCHAR test[],UCHAR kyoushi[],int dim)
{
  int i;
  float inp=0.0;

  for(i=0;i<dim;i++) {
      /* (E) この行を完成させる */
      inp += test[i]*kyoushi[i];
  }

  return inp;
}

/* 画素値を反転する(img1:反転後画像, img2:反転前画像) */
void hanten(UCHAR img1[][RETU_MOJI],UCHAR img2[][RETU_MOJI],int dim_gyou,int dim_retu)
{
  int g,r;

  for(g=0;g<dim_gyou;g++)
    for(r=0;r<dim_retu;r++)
      img1[g][r] = WHITE-img2[g][r];
}

/* 平均値がAVE_VALUEとなるように変換 */
void average_adjustment(UCHAR x[],double av,int n)
{
  int i;
  double sum=0.0;
  double ratio;
  int val;

  for(i=0;i<n;i++)
    sum += x[i];
  ratio=av/(sum/(double)n);

  for(i=0;i<n;i++)
  {
    val=(int)((double)x[i]*ratio+0.5);
    if(val > WHITE)
      x[i]=(UCHAR)WHITE;
    else
      x[i]=(UCHAR)val;
  }
}

/***** IDの読み込み *****/
int read_id_list(char id[][LEN_ID], char fname[])
{
  FILE *fp_list;
  char linebuf[200];
  int n=0;

  if((fp_list = fopen(fname, "rb")) == NULL)
    error1("Can't open idlist.txt");

  n=0;				/* 読み込んだIDの総数 */
  while(fgets(linebuf, sizeof(linebuf), fp_list) != NULL) {
    if(linebuf[0] == '#')	/* コメント行 */
      continue ;
    else if(linebuf[0] == '\n')	/* 改行のみ */
      continue ;
    else if(strncmp(linebuf, "$END", strlen("$END")) == 0) {
      break ;
    }

    if(linebuf[strlen(linebuf)-1] == '\n') /* IDの最後が改行の場合，改行を除く */
      linebuf[strlen(linebuf)-1] = '\0';

    strcpy(id[n], linebuf);
    n++;

    if(n >= N_ID)		/* IDの総数のチェック */
      error1("Number of ID is too large");
  }

  return n;
}

/* pgmフォーマットの画像ファイル読み込み(任意のサイズを可能とするためポインタ利用) */
void read_pgm(UCHAR **data_buf, char *fname,int *width,int *height)
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
  UCHAR *val;

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

  if((val=(UCHAR *)malloc((*width)*(*height)*sizeof(UCHAR))) == NULL)
    error1("Memory allocation failed in read_pgm.");

  /* 画像データ部分読み込み */
  k=0;
  fread(val, sizeof(UCHAR), (*width)*(*height), fp);
  for( m = 0; m < *height; m++)
    for( n = 0; n < *width; n++)
    {
      *((UCHAR *)data_buf+(m*(*width)+n)) = val[k++];
    }

  free(val);

  /* ファイルを閉じる */
  fclose(fp) ;
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


/* エラー処理 */
void error1(char *message)
{
  printf("%s\n",message);
  exit(1);
}
