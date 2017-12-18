/*************************************************************/
/*****                    toukei.c                         ***/
/*****    Usage: a.out                                     ***/
/*****    Function: 統計的識別(ベイズ推定，最尤推定)       ***/
/*****    教師，テストデータ: KL展開特徴量                 ***/
/*****    Output: 認識率                                   ***/
/*****    コンパイルオプション:                            ***/
/*****    -DSENKEI 線形識別関数を利用                      ***/
/*****    -DSOUKAN 相互相関を利用                          ***/
/*****    -DSAIYUU 最尤推定                                ***/
/*****    -DBAYES  ベイズ推定                              ***/
/*****                                                     ***/
/*************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef unsigned char UCHAR;

#define N_ID 121		/* 学習(orテスト)に使用するIDの最大数 */
#define LEN_ID 10		/* IDの最大文字数+1 */
#define GYOU_MOJI 100		/* 切り出す画像の行数 */
#define RETU_MOJI GYOU_MOJI	/* 切り出す画像の列数 */  
#define N_MOJI 10		/* 文字種の総数 */
#define TCH_SAMPLE 6		/* 各ID，各文字のデータ数の最大値 */
#define AVE_VALUE 20		/* 反転画像の平均画素値をこの値にそろえる(固定) */
#define BLACK 0			/* 黒の画素値 */
#define WHITE 255		/* 白の画素値 */
#define SHUKUSHOU_KEISUU 15	/* このサイズを一辺とする正方形を1画素に縮小 */
#define SIGMA_MIN 6		/* 標準偏差の初期値(最小値) */
#define SIGMA_MAX 6		/* 標準偏差の最終値(最大値) */
#define SIGMA_STEP 1		/* 標準偏差のステップ(変化量) */
#define MAXDIM_MIN 1		/* max_dim(KL変換後の利用次元数)の初期値(最小値) */
#define MAXDIM_MAX 1		/* max_dimの最終値(最大値) */
#define MAXDIM_STEP 1		/* max_dimのステップ(変化量) */

#define PATH_FOR_DATA "/mnt/oshare/2017/大宮月曜２限パターン認識 - 2311101030/配布用/Mae" /* データのパス */

#define KL_ID_FILENAME "./alllist.txt"  /* KL変換のために利用するIDのリスト */
#define KYOUSHI_ID_FILENAME "./alllist.txt"  /* 教師データとして利用するIDのリスト */
#define TEST_ID_FILENAME    "./alllist.txt" /* テストデータとして利用するIDのリスト */

#define VAL_DOUBLE(p,i1,i2,size) (*((double *)p+i1*size+i2))
#define VAL_UCHAR(p,i1,i2,size) (*((unsigned char *)p+i1*size+i2))
#define VAL_UCHAR3D(p,i1,i2,i3,size1,size2) (*((unsigned char *)p+(i1*size1+i2)*size2+i3))
#define	SWAP(a,b) {w=a;a=b;b=w;}
double w;

				/* 各ID,各文字について教師データに用いるデータ番号を指定 */
int sample_tch[]={0,1,2,3,4,5};
				/* 各ID,各文字についてテストデータに用いるデータ番号を指定 */
int sample_test[]={0,1,2,3,4,5};

int n_sample_tch=sizeof(sample_tch)/sizeof(int); /* 各ID,各文字についての教師データ数 */
int n_sample_test=sizeof(sample_test)/sizeof(int); /* 各ID,各文字についてのテストデータ数 */

void  calc_av_and_cvmat(double *, double *, UCHAR ***, int, int, int);
void  calc_av_and_cvmat_double(double *, double *, double **, int, int, int);
double calc_chujitudo(double [],int ,int );
void shukushou(UCHAR **, int, int, UCHAR [][RETU_MOJI],int ,int , int);
void kakudai(UCHAR [][RETU_MOJI],int ,int ,UCHAR **, int, int, int);
double calc_mahalanobis2(double [],double [],double *,int);
void calc_kl_component(double *,int,double *, double **,int);
void calc_inverse_kl(double *, double *, double **,int ,int );
void calc_eigen_vector(double **,double **,double *,int);
double calc_gyakugyouretu(double **,double **,int);
void ludcmp(double **,int,int *,double *);
void lubksb(double **,int,int *,double []);
double naiseki_double(double *, double *,int);
double calc_norm2_double(double [],int);
void jac(double **, double *, double **, int);
void sortev(double **, double *, double **, int);
int read_id_list(char [][LEN_ID],char []);
void hanten(UCHAR *,UCHAR *,int);
void average_adjustment(UCHAR [],double ,int );
void read_pgm(UCHAR **,char *,int *,int *);
void write_pgm(UCHAR **,char *,int ,int );
void error1(char *);


int main(int argc,char *argv[])
{
  int i,j,k,l,n;
  int in_retu,in_gyou;		/* 読み込んだ画像の列数，行数 */
  int shuku_gyou;		/* 縮小画像の行数 */
  int shuku_retu;		/* 縮小画像の列数 */
  int n_shuku;		/* 縮小画像をベクトルとみなした時の次元数 */
  int num_id_kl;		/* kl変換用ID(学籍番号)の総数 */
  int num_id_tch;		/* 教師データ用ID(学籍番号)の総数 */
  int num_id_test;		/* テストデータ用の総数 */
  char id_kl[N_ID][LEN_ID];	/* KL変換用データのID */
  char id_tch[N_ID][LEN_ID];	/* 教師データ用ID */
  char id_test[N_ID][LEN_ID];	/* テストデータ用ID */
  UCHAR kyoushi[GYOU_MOJI][RETU_MOJI]; /* 教師データ */
  UCHAR test[GYOU_MOJI][RETU_MOJI]; /* テストデータ */
  char in_fname[200];		/* 入力画像のファイル名 */
  int ans_number=0;		/* 算出された答え */
  double eval;			/* 評価値 */
  double eval_ans;		/* 算出された答えの評価値(距離，識別関数値など) */
  long n_moji;			/* あるIDのある文字についてのテスト回数 */
  long n_correct_moji;		/* そのうちの正解数 */
  long n_id;			/* あるIDの全文字についてのテスト回数 */
  long n_correct_id;		/* そのうちの正解数 */
  long n_total;			/* テスト用全IDの全文字についてのテスト回数 */
  long n_correct_total;		/* そのうちの正解数 */
  double rate;			/* 各IDの正解率 */
  double rate_max=0.0;		/* テストデータの各IDごとの正解率の最大値 */
  int id_rate_max=0;		/* その時のID番号 */
  double rate_min=0.0;		/* テストデータの各IDごとの正解率の最小値 */
  int id_rate_min=0;		/* その時のID番号 */
#ifdef SENKEI
  double bias[N_MOJI];		/* 線形識別関数用のバイアス値(ノルムの2乗/2) */
#endif
#ifdef SOUKAN
  double norm_kyoushi[N_MOJI];	/* 各文字の教師データのノルム(ベクトル長) */
  double norm_test;		/* テストデータのノルム */
#endif
  int sigma;			/* 平滑化時のガウシアン分布の標準偏差 */
				/* 本来は実数だが整数を使うことにする */
  int n_kl_moji;		/* 平均値，共分散行列を算出するパターンの総数/文字 */
  int n_tch_moji;		/* 教師パターンの総数/文字 */
  int n_kl_all;			/* 平均値，共分散行列を算出するパターンの総数 */
  int n_tch_all;		/* 教師パターンの総数 */
  int max_dim;			/* KL展開の次元数 */


  shuku_gyou=(int)GYOU_MOJI/(int)SHUKUSHOU_KEISUU;
  if(shuku_gyou*SHUKUSHOU_KEISUU != GYOU_MOJI) /* 割切れない場合 */
    shuku_gyou++;
  shuku_retu=(int)RETU_MOJI/(int)SHUKUSHOU_KEISUU;
  if(shuku_retu*SHUKUSHOU_KEISUU != RETU_MOJI) /* 割切れない場合 */
    shuku_retu++;
  n_shuku=shuku_gyou*shuku_retu; /* 縮小画像をベクトルと見なした時の次元数算出 */
  printf("shuku_gyou=%d shuku_retu=%d n_shuku=%d\n",shuku_gyou,shuku_retu,n_shuku);

  /***** IDの読み込み *****/
  num_id_kl=read_id_list(id_kl, KL_ID_FILENAME);/* kl変換用ID読み込み */
  printf("Number of ID for kl henkan = %d\n",num_id_kl);
  num_id_tch=read_id_list(id_tch, KYOUSHI_ID_FILENAME);/* 教師データ用ID読み込み */
  printf("Number of ID for tch = %d\n",num_id_tch);
  num_id_test=read_id_list(id_test, TEST_ID_FILENAME);/* テストデータ用ID読み込み */
  printf("Number of ID for test = %d\n",num_id_test);

  for(sigma=SIGMA_MIN;sigma<=SIGMA_MAX;sigma+=SIGMA_STEP)
  {
    UCHAR kyoushi_shuku[N_MOJI][N_ID*TCH_SAMPLE][n_shuku]; /* 縮小後のパターン(ベクトルとみなす) */
    double kyoushi_tmp[n_shuku]; /* 縮小後のパターン(実数型変数) */
    double kyoushi_dbl[N_ID*TCH_SAMPLE][n_shuku]; /* KL変換後のパターン */
    double kyoushi_av[N_MOJI][n_shuku]; /* 教師パターンの平均のKL変換成分 */
    UCHAR test_shuku[n_shuku];	/* 縮小後のテストパターン(ベクトルとみなす) */
    double test_dbl[n_shuku];	/* 縮小後のテストパターン(実数型変数) */
    double test_cmp[n_shuku];	/* 縮小後のテストパターンのKL変換成分 */
    double av[n_shuku];		/* 平均値計算用メモリ */
    double cvmat[n_shuku*n_shuku]; /* 共分散行列計算用メモリ */
    double vec[n_shuku][n_shuku]; /* 固有ベクトル */
    double lambda[n_shuku];	/* 固有値 */
    double inv_cvmat[N_MOJI][n_shuku*n_shuku]; /* 縮小後の教師パターンの共分散行列の逆行列 */
    double det[N_MOJI];		/* 共分散行列の行列式の値 */
    double log_det[N_MOJI];	/* 共分散行列の行列式の値の自然対数 */
#ifdef BAYES
    double log_pw[N_MOJI];	/* 各カテゴリの出現確率の自然対数 */
#endif
    
    printf("sigma=%d\n",sigma);

    /* KL変換開始 */
    /* (手順一)KL変換に用いるパターンを読み，反転，縮小してメモリに保存 */
    n_kl_all=0;
    for(i=0;i<N_MOJI;i++)
    {
      n_kl_moji=0;			
      for(n=0;n<num_id_kl;n++) /* 全パターンの加工，縮小パターンをメモリに保存 */
	for(j=0;j<TCH_SAMPLE;j++) /* KL変換にはそのIDの全パターンを用いる */
	{
	  sprintf(in_fname,"%s/%s/SIGMA%d/%smae-%1d-%1d.pgm",PATH_FOR_DATA,id_kl[n],sigma,id_kl[n],i,j); 
	  read_pgm((UCHAR **)kyoushi, in_fname, &in_retu, &in_gyou);
	  if((in_retu != RETU_MOJI) || (in_gyou != GYOU_MOJI))
	    error1("Dimensions are wrong.");

				/* 画素値を反転する(文字が無い場合を原点とするため) */
	  hanten((UCHAR *)kyoushi, (UCHAR *)kyoushi, in_gyou*in_retu);
				/* 反転した画像の画素値の平均をAVE_VALUEにそろえる */
	  average_adjustment((UCHAR *)kyoushi, AVE_VALUE, in_retu*in_gyou);

	  shukushou((UCHAR **)kyoushi_shuku[i][n_kl_moji], shuku_gyou, shuku_retu, kyoushi, in_gyou, in_retu, SHUKUSHOU_KEISUU); /* 縮小する(パターンは1次元ベクトルになる) */

	  n_kl_moji++;
	  n_kl_all++;
	}
    }

    /* (手順二)KL展開のための固有値，固有ベクトル算出 */
    /* 縮小ベクトルkyoushi_shuku[](UCHAR型)全てを用いて平均値と共分散行列算出 */
    calc_av_and_cvmat(av,cvmat,(UCHAR ***)kyoushi_shuku,(int)N_MOJI,n_kl_moji,n_shuku);

				/* 共分散行列を元に固有値，固有ベクトル算出 */
				/* 固有ベクトルvecは固有値の大きな順番になっている． */
    calc_eigen_vector((double **)vec,(double **)cvmat,lambda,n_shuku);

				/* KL展開のための固有ベクトル，次元の計算完了 */

    /* (手順三)確率パラメータ(平均値，共分散行列の逆行列)の作成処理 */
    /* 教師パターンを読み，縮小してメモリに保存 */
    n_tch_all=0;
    for(i=0;i<N_MOJI;i++)
    {
      n_tch_moji=0;			
      for(n=0;n<num_id_tch;n++) 
	for(j=0;j<n_sample_tch;j++)
	{			/* 教師データのファイル名 */
	  sprintf(in_fname,"%s/%s/SIGMA%d/%smae-%1d-%1d.pgm",PATH_FOR_DATA,id_tch[n],sigma,id_tch[n],i,sample_tch[j]); 
	  read_pgm((UCHAR **)kyoushi, in_fname, &in_retu, &in_gyou);
	  if((in_retu != RETU_MOJI) || (in_gyou != GYOU_MOJI))
	    error1("Dimensions are wrong.");

				/* 画素値を反転する(文字が無い場合を原点とするため) */
	  hanten((UCHAR *)kyoushi, (UCHAR *)kyoushi, in_gyou*in_retu);
				/* 反転した画像の画素値の平均をAVE_VALUEにそろえる */
	  average_adjustment((UCHAR *)kyoushi, AVE_VALUE, in_retu*in_gyou);

	  shukushou((UCHAR **)kyoushi_shuku[i][n_tch_moji], shuku_gyou, shuku_retu, kyoushi, in_gyou, in_retu, SHUKUSHOU_KEISUU); /* 縮小する(パターンは1次元ベクトルになる) */

	  n_tch_moji++;
	  n_tch_all++;
	}
    }

    /* これ以降，利用する次元数を変えながら処理を繰り返す */
    for(max_dim=MAXDIM_MIN;max_dim<=MAXDIM_MAX;max_dim+=MAXDIM_STEP)
    {
      printf("max_dim=%d chujitudo=%f\n",max_dim,calc_chujitudo(lambda,n_shuku,max_dim));
      //      printf("%3d ,%f ",max_dim,calc_chujitudo(lambda,n_shuku,max_dim));

      /* 各カテゴリ(数字)について確率パラメータを算出する */
      for(i=0;i<N_MOJI;i++)
      {
	for(l=0;l<n_tch_moji;l++) /* 各教師パターンをKL変換(とりあえず全次元を保存) */
	{
	  for(k=0;k<n_shuku;k++)	/* UCHAR型から実数型へコピー */
	    kyoushi_tmp[k]=(double)kyoushi_shuku[i][l][k];
				/* KL変換成分算出 */
	  calc_kl_component(kyoushi_dbl[l], n_shuku, kyoushi_tmp, (double **)vec, n_shuku);
	}

				/* KL変換成分のうちmax_dim次元を用い，カテゴリの平均値と共分散行列算出 */
	calc_av_and_cvmat_double(kyoushi_av[i],cvmat,(double **)kyoushi_dbl,n_tch_moji,max_dim,n_shuku);

				/* 共分散行列の逆行列と行列式の値を算出 */
	det[i]=calc_gyakugyouretu((double **)inv_cvmat[i],(double **)cvmat,max_dim);
	if(det[i] <= 0.000000001)
	{
	  printf("Number = %d\n",i);
	  error1("Can't calculate Inverse matrix!\n");
	}
	
	log_det[i]=log(det[i]);	/* 行列式の値については自然対数も予め算出(計算時間短縮のため) */
      }
      /* カテゴリ i の平均値av[i][]，共分散行列の逆行列inv_cvmat[i][]，行列式の値det[i]の算出終了 */

    /* (手順四)テスト開始 */
#ifdef SENKEI
      for(i=0;i<N_MOJI;i++)	/* 線形識別関数で用いるバイアス値(ノルムの2乗/2)算出 */
	bias[i]=calc_norm2_double(kyoushi_av[i], max_dim)/2.0;
#endif

#ifdef SOUKAN
      for(i=0;i<N_MOJI;i++)	/* 相互相関で用いる教師データのノルム算出 */
	norm_kyoushi[i]=sqrt(calc_norm2_double(kyoushi_av[i], max_dim));
#endif

#ifdef BAYES	/* (A)ベイズ識別で用いる項log_pw[カテゴリ]を算出する処理を完成する */
				/* log_pw[i]はカテゴリiの出現確率の自然対数 */
      for(i=0;i<N_MOJI;i++)
      {
	  log_pw[i]=
      }
#endif
    
      rate_max=-1.0;		/* rate_max,rate_minの初期化 */
      rate_min=101.0;
      n_correct_total=0;	/* そのIDの全データでの正解数を初期化 */
      n_total=0;
      for(n=0;n<num_id_test;n++)
      {
	n_correct_id=0;		/* そのIDの全データでの正解数を初期化 */
	n_id=0;
	for(i=0;i<N_MOJI;i++)	  
	{
#ifndef STOP_DETAIL
	  printf("%2d  ",i);
#endif
	  n_correct_moji=0;	/* 各文字の正解数を初期化 */
	  n_moji=0;
	  /* (E) */
	  for(j=0;j<n_sample_test;j++) /* (a)出現確率は同じ */
	  //	  for(j=0;j<n_sample_test;j+=(1+(i/2==(i+1)/2)*5)) /* (b)偶数の出現確率を奇数の1/6にする */
	  //	  for(j=0;j<n_sample_test;j+=(1+(i/2!=(i+1)/2)*5)) /* (c)奇数の出現確率を偶数の1/6にする */
	  {			/* テスト画像ファイル名 */
	    sprintf(in_fname,"%s/%s/SIGMA%d/%smae-%1d-%1d.pgm",PATH_FOR_DATA,id_test[n],sigma,id_test[n],i,sample_test[j]); 
	    read_pgm((UCHAR **)test, in_fname, &in_retu, &in_gyou);  /* テスト画像読み込み */
	    if((in_retu != RETU_MOJI) || (in_gyou != GYOU_MOJI))
	      error1("Dimensions are wrong.");

	    hanten((UCHAR *)test, (UCHAR *)test, in_gyou*in_retu); /* 画素値を反転する */

	    average_adjustment((UCHAR *)test, AVE_VALUE, in_retu*in_gyou); /* 平均値をAVE_VALUEに規格化 */

	    shukushou((UCHAR **)test_shuku, shuku_gyou, shuku_retu, test, in_gyou, in_retu, SHUKUSHOU_KEISUU); /* 縮小する */

	    for(l=0;l<n_shuku;l++) /* 後の関数用にUCHAR型から実数型へコピー */
	      test_dbl[l]=(double)test_shuku[l];
				/* 部分空間成分を求める */	
	    calc_kl_component(test_cmp, max_dim, test_dbl, (double **)vec, n_shuku);

#ifdef SOUKAN
	    norm_test=sqrt(calc_norm2_double(test_cmp, max_dim));
#endif
	    eval_ans=-GYOU_MOJI*RETU_MOJI*WHITE*WHITE-1.0; /* 評価値を考えられる最小値に設定 */
	    for(k=0;k<N_MOJI;k++) /* 評価値算出 */
	    {	
#ifdef SENKEI
	      eval=naiseki_double(test_cmp, kyoushi_av[k], max_dim)-bias[k];
#endif
#ifdef SOUKAN
	      norm_test=sqrt(calc_norm2_double(test_cmp, max_dim));
	      eval=naiseki_double(test_cmp, kyoushi_av[k], max_dim)/norm_test/norm_kyoushi[k];
#endif
#ifdef SAIYUU			/* (B)最尤法の識別関数を完成する */
	      eval=-0.5*
#endif
#ifdef BAYES			/* (C)ベイズ識別法の識別関数を完成する */
	      eval=-0.5*
#endif
	      if(eval > eval_ans) /* 評価値が最大となる数字kを保存 */
	      {
		eval_ans=eval;
		ans_number=k;
	      }
	    }
	    if(ans_number == i)	/* 正解の場合 */
	    {
#ifndef STOP_DETAIL
	      printf("  ");
#endif
	      n_correct_moji++;
	    }
	    else
	    {
#ifndef STOP_DETAIL
	      printf("%1d ",ans_number);
#endif
	    }
	    n_moji++;
	  }
#ifndef STOP_DETAIL		/* 各IDの各文字の正解率表示 */
	  printf("%ld/%ld (%6.2f%%)\n",n_correct_moji,n_moji,(double)n_correct_moji/(double)n_moji*100.0); 
#endif
	  n_correct_id += n_correct_moji;
	  n_id += n_moji;
	}
	rate=(double)n_correct_id/(double)n_id*100.0;
#ifndef STOP_DETAIL
	printf("ID=%s %ld/%ld (%6.2f%%)\n\n",id_test[n],n_correct_id,n_id,rate); /* 各IDでの正解率表示 */
#endif
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
      }                /* 全データでの正解率表示 */
      printf("TOTAL %ld/%ld (%6.2f%%)\n",n_correct_total,n_total,(double)n_correct_total/(double)n_total*100.0);
      //      printf(",%6.2f\n",(double)n_correct_total/(double)n_total*100.0);
#ifndef STOP_DETAIL
      printf("Best ID  = %s (%6.2f%%)\n",id_test[id_rate_max],rate_max); /* 正解率最大，最小IDの表示 */
      printf("Worst ID = %s (%6.2f%%)\n\n",id_test[id_rate_min],rate_min);
#endif
    }
  } /* sigma */
  return 0;
}

/* マハラノビス距離の2乗を返す */
double calc_mahalanobis2(double x[],double av[],double *inv,int n)
{
  double sum=0.0;
  double tmp;
  int i,j;
  
/* 変数の説明 */
/* x[] : データベクトル */
/* av[] : 平均ベクトル */
/* *inv : 共分散行列の逆行列(nxn次元)（行列だが最初から１次元配列として確保） */
/* n : ベクトル,行列の次元  */

  for(i=0;i<n;i++)
  {
    tmp=0.0;			/* VAL_DOUBLE()を利用する */
    for(j=0;j<n;j++)		/* (D)マハラノビス距離の2乗を算出する処理を完成する */
      tmp += 
    sum += 
  }
  
  return sum;
}

/* 縮小ベクトルkyoushi_shuku[](UCHAR型)全てを用いて平均値と共分散行列算出 */
void calc_av_and_cvmat(double *av, double *cvmat, UCHAR ***kyoushi_shuku, int n_moji, int n_kl_moji, int n_shuku)
{
  int i,l,k,k1,k2;
  int n_all=n_moji*n_kl_moji;

  /* 平均値算出 */  
  for(k=0;k<n_shuku;k++)
    av[k]=0.0;

  for(i=0;i<N_MOJI;i++)
    for(l=0;l<n_kl_moji;l++)
      for(k=0;k<n_shuku;k++) 
	av[k] += VAL_UCHAR3D(kyoushi_shuku,i,l,k,N_ID*TCH_SAMPLE,n_shuku);

  for(k=0;k<n_shuku;k++)
    av[k] /= (double)n_all;


    /* 共分散行列算出 */
  for(k1=0;k1<n_shuku;k1++)	/* 配列を0に初期化 */
    for(k2=0;k2<n_shuku;k2++)
      VAL_DOUBLE(cvmat,k1,k2,n_shuku)=0.0;

  for(i=0;i<N_MOJI;i++)		/* 全データ(n_all=n_moji*n_kl_moji)について */
    for(l=0;l<n_kl_moji;l++)
      for(k1=0;k1<n_shuku;k1++)	/* 2つの次元の組み合わせについて，平均からの差分の積を加算 */
	for(k2=k1;k2<n_shuku;k2++)
	  VAL_DOUBLE(cvmat,k1,k2,n_shuku) += ((double)VAL_UCHAR3D(kyoushi_shuku,i,l,k1,N_ID*TCH_SAMPLE,n_shuku)-av[k1])*((double)VAL_UCHAR3D(kyoushi_shuku,i,l,k2,N_ID*TCH_SAMPLE,n_shuku)-av[k2]);

  /* 加算した配列(VAL_DOUBLE(cvmat,k1,k2,n_shuku))を加算回数で割ってcvmatを完成させる */
  for(k1=0;k1<n_shuku;k1++)
    for(k2=k1;k2<n_shuku;k2++)
      VAL_DOUBLE(cvmat,k1,k2,n_shuku)=VAL_DOUBLE(cvmat,k2,k1,n_shuku)=VAL_DOUBLE(cvmat,k1,k2,n_shuku)/(double)n_all;
}


/* 縮小ベクトルkyoushi_dbl[]全てを用いて平均値と共分散行列算出 */
void calc_av_and_cvmat_double(double *av, double *cvmat, double **kyoushi_dbl, int n_tch_moji, int max_dim, int n_shuku)
{
  int i,k,k1,k2;

  /* 平均値算出 */  
  for(k=0;k<max_dim;k++)
    av[k]=0.0;

  for(i=0;i<n_tch_moji;i++)
    for(k=0;k<max_dim;k++) 
      av[k] += VAL_DOUBLE(kyoushi_dbl,i,k,n_shuku);

  for(k=0;k<max_dim;k++)
    av[k] /= (double)n_tch_moji;


    /* 共分散行列算出 */
  for(k1=0;k1<max_dim;k1++)	/* 配列を0に初期化 */
    for(k2=0;k2<max_dim;k2++)
      VAL_DOUBLE(cvmat,k1,k2,max_dim)=0.0;

  for(i=0;i<n_tch_moji;i++)	/* 全データ(n_tch_moji)について */
    for(k1=0;k1<max_dim;k1++)	/* 2つの次元の組み合わせについて，平均からの差分の積を加算 */
      for(k2=k1;k2<max_dim;k2++) 
	VAL_DOUBLE(cvmat,k1,k2,max_dim) += (VAL_DOUBLE(kyoushi_dbl,i,k1,n_shuku)-av[k1])*(VAL_DOUBLE(kyoushi_dbl,i,k2,n_shuku)-av[k2]);

  /* 加算した配列(VAL_DOUBLE(cvmat,k1,k2,max_dim))を加算回数で割ってcvmatを完成させる */
  for(k1=0;k1<max_dim;k1++)
    for(k2=k1;k2<max_dim;k2++)
      VAL_DOUBLE(cvmat,k1,k2,max_dim)=VAL_DOUBLE(cvmat,k2,k1,max_dim)=VAL_DOUBLE(cvmat,k1,k2,max_dim)/(double)n_tch_moji;
}


/* KL変換成分を求める */
void calc_kl_component(double *kl,int dim,double *a, double **vec,int n)
{
  double tmp[dim];		/* KL変換成分を一時保存する配列(*klと*aが同じ場合のために必要) */
  int d;

/* 変数の説明 */
/* *kl : KL変換係数を代入する配列*/
/* dim : KL変換係数を算出する最大次元数(=max_dim) */
/* *a : KL変換すべき元データの配列 */
/* **vec : 固有ベクトル */
/* n : 固有ベクトルの次元数かつ固有ベクトルの数  */

/*(ヒント)*/
/* 本来はvec[0]が固有値の一番大きな固有ベクトル(の先頭アドレス)を表すが， */
/* この関数内ではvecは2次元配列の先頭アドレス**vecとして定義されているため */
/* vec[0], vec[0][0]のような表記は利用できない. */
/* そこで，VAL_DOUBLEなどと同様に，アドレスの計算，型のキャストを利用する． */

  for(d=0;d<dim;d++)
    tmp[d]=naiseki_double(a,(double *)((double *)vec+d*n),n);
  
  for(d=0;d<dim;d++)
    kl[d]=tmp[d];
}

/* 内積算出 */
double naiseki_double(double *a, double *b,int n)
{
  double sum=0.0;
  int i;
  
  for(i=0;i<n;i++)
    sum += a[i]*b[i];
  
  return sum;
}

/* KL逆変換 */
void calc_inverse_kl(double *a, double *kl, double **vec,int n,int dim)
{
  int d;
  int i;
  
  for(d=0;d<n;d++)
    a[d]=0.0;
  
  for(i=0;i<dim;i++)
    for(d=0;d<n;d++)
      a[d] += kl[i]*VAL_DOUBLE(vec,i,d,n);
}

/* パターンのノルム(ベクトルの長さ)の自乗を算出 */
double calc_norm2_double(double g_data[],int dim)
{
  int i;
  double sum=0.0;

  for(i=0;i<dim;i++)
    sum += g_data[i]*g_data[i];

  return sum;
}

/* 縮小する */
void shukushou(UCHAR **shuku_data,int shuku_gyou,int shuku_retu, UCHAR g_data[][RETU_MOJI],int dim_gyou,int dim_retu, int n_size)
{
  int g,r;
  int g2,r2;
  int sum[shuku_gyou][shuku_retu];
  int n_add[shuku_gyou][shuku_retu];

  for(g=0;g<shuku_gyou;g++)
    for(r=0;r<shuku_retu;r++)
    {
      sum[g][r]=0;
      n_add[g][r]=0;
    }

  for(g=0;g<dim_gyou;g++)
  {
    g2=g/n_size;
    for(r=0;r<dim_retu;r++)
    {
      r2=r/n_size;
      sum[g2][r2] += g_data[g][r];
      n_add[g2][r2]++;
    }
  }

  for(g=0;g<shuku_gyou;g++)
    for(r=0;r<shuku_retu;r++)
      VAL_UCHAR(shuku_data,g,r,shuku_retu)=(UCHAR)((double)sum[g][r]/(double)n_add[g][r]+0.4999);
}

/* 拡大する */
void kakudai(UCHAR g_data[][RETU_MOJI],int dim_gyou,int dim_retu, UCHAR **shuku_data,int shuku_gyou,int shuku_retu, int n_size)
{
  int g,r;
  int g2,r2;
  
  for(g=0;g<dim_gyou;g++)
  {
    g2=g/n_size;
    for(r=0;r<dim_retu;r++)
    {
      r2=r/n_size;
      g_data[g][r] = VAL_UCHAR(shuku_data,g2,r2,shuku_retu);
    }
  }
}

/* パターンのノルム(ベクトルの長さ)の自乗を算出 */
double calc_norm2(UCHAR g_data[],int dim)
{
  int i;
  double sum=0.0;

  for(i=0;i<dim;i++)
    sum += g_data[i]*g_data[i];

  return sum;
}

/* 内積を算出 */
double naiseki(UCHAR test[],UCHAR kyoushi[],int dim)
{
  int i;
  double sum=0.0;

  for(i=0;i<dim;i++)
    sum += test[i]*kyoushi[i];

  return sum;
}

/* 画素値を反転する */
void hanten(UCHAR *g_data1,UCHAR *g_data2,int dim)
{
  int i;

  for(i=0;i<dim;i++)
    g_data1[i]=WHITE-g_data2[i];
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


/* 固有値，固有ベクトルの計算 */
void calc_eigen_vector(double **ans,double **a,double *lambda,int n)
{
  int i,j;
  
      jac(a,lambda,ans,n);
      sortev(a,lambda,ans,n);
				/* ansの行と列を交換 */
      for ( i=0 ; i<n ; ++i )	/* (ans[i]が一つの固有ベクトルを意味するように) */
	for ( j=i ; j<n ; ++j )
	  SWAP(VAL_DOUBLE(ans,i,j,n),VAL_DOUBLE(ans,j,i,n))

	    /**
      for ( i=0 ; i<n ; ++i )
      {
         printf("e[%d]=%f\n",i,lambda[i]);
         printf(" 固有ベクトル \n");
	 for ( j=0 ; j<n ; ++j )
            printf("%f\n", (double)VAL_DOUBLE(ans,i,j,n));
      }
	    **/
}

/*   ヤコビ法による固有値計算   */
void jac(double **a, double *e, double **v, int n)
{
  int    i,j,kmax=n,kaisuu;
  int    p,q;
  double eps,eps2,bunbo,c,s;
  double apq,apqmax,gmax,wa,sa;
  double apj,aqj,vip,viq;

  /* 収束判定定数の算定 */
  gmax=0.0;
  for ( i=0 ; i<n ; ++i )
  {
    s=0.0;
    for ( j=0 ; j<n ; ++j )
      s+=fabs(VAL_DOUBLE(a,i,j,n));
    if (s>gmax) gmax=s;
  }
  eps=0.000001*gmax;
  eps2=0.001*eps;
  /* 固有ベクトルの計算の準備 */
  for ( i=0 ; i<n ; ++i )
  {
    for ( j=0 ; j<n ; ++j )
      VAL_DOUBLE(v,i,j,n)=0.0;
    VAL_DOUBLE(v,i,i,n)=1.0;
  }
  /* メイン・ループ */
  for ( kaisuu=1 ; kaisuu<=kmax ; ++kaisuu )
  {
    /* 収束判定 */
    apqmax=0.0;
    for ( p=0 ; p<n-1 ; ++p )
    {
      for ( q=p+1 ; q<n ; ++q )
      {
	apq=fabs(VAL_DOUBLE(a,p,q,n));
	if (apq>apqmax) apqmax=apq;
      }
    }
    if (apqmax<eps) break;
    /* 走査 */
    for ( p=0 ; p<n-1 ; ++p )
    {
      for ( q=p+1 ; q<n ; ++q )
      {
	apq=VAL_DOUBLE(a,p,q,n);
	if (fabs(apq=apq)<eps) break;
	/* ｓとｃの算定 */
	wa=(VAL_DOUBLE(a,p,p,n)+VAL_DOUBLE(a,q,q,n))*0.5;
	sa=(VAL_DOUBLE(a,p,p,n)-VAL_DOUBLE(a,q,q,n))*0.5;
	bunbo=sqrt(sa*sa+VAL_DOUBLE(a,p,q,n)*VAL_DOUBLE(a,p,q,n));
	if ( sa>0.0 )
	{
	  c=sqrt(1.0+sa/bunbo)/1.41421356;
	  s=apq/(2.0*c*bunbo);
	  /* 対角要素 */
	  VAL_DOUBLE(a,p,p,n)=wa+bunbo;
	  VAL_DOUBLE(a,q,q,n)=wa-bunbo;
	}
	else
	{
	  c=sqrt(1.0-sa/bunbo)/1.41421356;
	  s=(-apq)/(2.0*c*bunbo);
	  /* 対角要素 */
	  VAL_DOUBLE(a,p,p,n)=wa-bunbo;
	  VAL_DOUBLE(a,q,q,n)=wa+bunbo;
	}
	/* 非対角要素 */
	VAL_DOUBLE(a,p,q,n)=VAL_DOUBLE(a,q,p,n)=0.0;
	for ( j=0 ; j<p ; ++j )
	{
	  apj=VAL_DOUBLE(a,p,j,n);
	  aqj=VAL_DOUBLE(a,q,j,n);
	  VAL_DOUBLE(a,j,p,n)=VAL_DOUBLE(a,p,j,n)=  apj*c+aqj*s;
	  VAL_DOUBLE(a,j,q,n)=VAL_DOUBLE(a,q,j,n)=(-apj*s+aqj*c);
	}
	for ( j=p+1 ; j<q ; ++j )
	{
	  apj=VAL_DOUBLE(a,p,j,n);
	  aqj=VAL_DOUBLE(a,q,j,n);
	  VAL_DOUBLE(a,j,p,n)=VAL_DOUBLE(a,p,j,n)=  apj*c+aqj*s;
	  VAL_DOUBLE(a,j,q,n)=VAL_DOUBLE(a,q,j,n)=(-apj*s+aqj*c);
	}
	for ( j=q+1 ; j<n ; ++j )
	{
	  apj=VAL_DOUBLE(a,p,j,n);
	  aqj=VAL_DOUBLE(a,q,j,n);
	  VAL_DOUBLE(a,j,p,n)=VAL_DOUBLE(a,p,j,n)=  apj*c+aqj*s;
	  VAL_DOUBLE(a,j,q,n)=VAL_DOUBLE(a,q,j,n)=(-apj*s+aqj*c);
	}
	for ( i=0 ; i<n ; ++i )
	{
	  vip=VAL_DOUBLE(v,i,p,n);
	  viq=VAL_DOUBLE(v,i,q,n);
	  VAL_DOUBLE(v,i,p,n)=  vip*c+viq*s;
	  VAL_DOUBLE(v,i,q,n)=(-vip*s+viq*c);
	}
      }
    }
    eps=eps*1.05;
  }
  for ( i=0 ; i<n ; ++i )
    e[i]=VAL_DOUBLE(a,i,i,n);
}

/*   固有値の整列   */
void sortev(double **a, double *e, double **v, int n)
{
  int    i,j,k;
  double w;
  for ( i=0 ; i<n-1 ; ++i )
  {
    k=i;
    for ( j=i+1 ; j<n ; ++j )
      if (fabs(e[j])>fabs(e[k])) k=j;
    if ( k!=i )
    {
      SWAP(e[i],e[k]);
      for ( j=0 ; j<n ; ++j )
	SWAP(VAL_DOUBLE(v,j,i,n),VAL_DOUBLE(v,j,k,n));
    }
  }
}


/* 逆行列算出．行列式の値を返す */
double calc_gyakugyouretu(double **ans,double **a,int n)
{
  double y[n][n];
  double col[n];
  double d;
  int indx[n];
  int i,j;
  
  ludcmp(a,n,indx,&d);

  for(j=0;j<n;j++)
    d *= VAL_DOUBLE(a,j,j,n);
  
  for(j=0;j<n;j++)
  {
    for(i=0;i<n;i++)
      col[i]=0.0;
    col[j]=1.0;
    lubksb(a,n,indx,col);
    for(i=0;i<n;i++)
      y[i][j]=col[i];
  }

  for(i=0;i<n;i++)
    for(j=0;j<n;j++)
      VAL_DOUBLE(ans,i,j,n)=y[i][j];
  
  return d;
}



/* LU分解 */
void ludcmp(double **a, int n, int *indx, double *d)
{
  int i,imax=0,j,k;
  double big,dum,sum,temp;
  double  vv[n];

  *d=1.0;
  for(i=0;i<n;i++)
  {
    big=0.0;
    for(j=0;j<n;j++)
      if((temp=fabs(VAL_DOUBLE(a,i,j,n))) > big) 
	big=temp;
    if(big == 0.0)
      error1("Singular matrix in routine ludcmp");
    vv[i]=1.0/big;
  }
  for(j=0;j<n;j++)
  {
    for(i=0;i<j;i++)
    {
      sum=VAL_DOUBLE(a,i,j,n);
      for(k=0;k<i;k++)
	sum -= VAL_DOUBLE(a,i,k,n)*VAL_DOUBLE(a,k,j,n);
      VAL_DOUBLE(a,i,j,n)=sum;
    }
    big=0.0;
    for(i=j;i<n;i++)
    {
      sum=VAL_DOUBLE(a,i,j,n);
      for(k=0;k<j;k++)
	sum -= VAL_DOUBLE(a,i,k,n)*VAL_DOUBLE(a,k,j,n);
      VAL_DOUBLE(a,i,j,n)=sum;
      if((dum=vv[i]*fabs(sum)) >= big)
      {
	big=dum;
	imax=i;
      }
    }
    if(j != imax)
    {
      for(k=0;k<n;k++)
      {
	dum=VAL_DOUBLE(a,imax,k,n);
	VAL_DOUBLE(a,imax,k,n)=VAL_DOUBLE(a,j,k,n);
	VAL_DOUBLE(a,j,k,n)=dum;
      }
      *d=-(*d);
      vv[imax]=vv[j];
    }
    indx[j]=imax;
    if(fabs(VAL_DOUBLE(a,j,j,n)) < 0.00000000000000000001)
      (VAL_DOUBLE(a,j,j,n))=0.00000000000000000001;
    if(j != (n-1))
    {
      dum=1.0/VAL_DOUBLE(a,j,j,n);
      for(i=j+1;i<n;i++)
	VAL_DOUBLE(a,i,j,n) *= dum;
    }
  }
}

/* LU分解された行列を用いて連立方程式を解く */
void lubksb(double **a, int n, int *indx, double b[])
{
  int i,ii=-1,ip,j;
  double sum;

  for(i=0;i<n;i++)
  {
    ip=indx[i];
    sum=b[ip];
    b[ip]=b[i];
    if(ii>=0)
      for(j=ii;j<=i-1;j++)
	sum -= VAL_DOUBLE(a,i,j,n)*b[j];
    else if (sum)
      ii=i;
    b[i]=sum;
  }
  for(i=n-1;i>=0;i--)
  {
    sum=b[i];
    for(j=i+1;j<n;j++)
      sum -= VAL_DOUBLE(a,i,j,n)*b[j];
    b[i]=sum/VAL_DOUBLE(a,i,i,n);
  }
}

/* max_dim次元までの忠実度を算出する */
double calc_chujitudo(double lambda[],int n_shuku,int max_dim)
{
  double sum=0.0;
  double wa=0.0;
  int i;
  
  if(max_dim > n_shuku)
    error1("Irregal max_dim in calc_chujitudo()");
  
  for(i=0;i<n_shuku;i++)	/* 固有値の総和 */
    wa += lambda[i];
  
  for(i=0;i<max_dim;i++)
    sum += lambda[i];
  
  return (double)(sum/wa);
}

