/*************************************************************/
/*****                    backprop.c                       ***/
/*****    Usage: a.out                                     ***/
/*****    Function: BPによる学習，テスト                   ***/
/*****    Output: 誤差，重みファイル(学習時)               ***/
/*****            認識率(テスト時)                         ***/
/*****                                                     ***/
/*************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MaxLayers 3		/* 最大層数 */
#define MaxNodes 50		/* 各層の最大ニューロン数 */
#define SEED 200		/* 乱数の種の初期値(基本的に学籍番号の下3桁) */

#define MaxDataFileNum 121	/* データファイルの最大数 */
#define MaxDataNum MaxDataFileNum*60 /* 学習パターンの最大数 */

#define MaxDataNameLength 200	/* データファイル名の最大長 */
#define TxtLineLen 2000		/* ファイルからのデータ読み込み時の１行あたりの最大文字数 */
#define LEARN 0			/* 学習 */
#define TEST 1			/* テスト */
#define OK 1			/* OK */
#define NO 0			/* NO */

char *ident="test";		/* ident */
				/* この文字列XXXXに対してbpXXXX.parが読まれ，bpXXXX.woutが作成される */

typedef struct {
  int 	learn_or_test;		/* 0:learn 1:test */
  int 	layers;			/* layer数 */
  int	o_layer;		/* 出力層のlayer番号 */
  int	hidden_nodes1;		/* 第１かくれ層ニューロンの数 */
  float weight_init_range;	/* 重みを一様乱数で初期設定する際の最大値 */
  int	weight_init_mode;	/* 重みを初期設定するモード */
  int	max_cycles;		/* パターン当りの最大学習回数 */
  float error_conv;		/* 収束と判定する２乗平均誤差値 */
  float epsilon;		/* 学習レート */
  float alpha;			/* 慣性係数 */
  int	random_order;		/* 学習順序をランダムにするかどうか */
  int	n_datafile;		/* データファイル数 */
} Parstruct;


/* 学習，テスト関連の関数 */
void set_nodes_of_layer();	/* nodes_of_layer設定 */
void calc_feedforward();	/* 各層のニューロンの出力計算 */
void calc_dw();			/* 重み更新量計算 */
void update_w();		/* 重み更新 */
float calc_error();		/* ２乗平均エラー計算 */
void init_pattern_order();	/* パターンの学習順序を初期化 */
void randomize_pattern_order();	/* pattern_order[]をランダマイズ */
void init_random();		/* 乱数生成関数rnum()で使用する変数ransuuを初期化 */
int rnum(int );			/* ０からrange-1の間の一様乱数を返す */

/* 重み入出力関連の関数 */
void init_weight_by_ransuu();	/* 重みを一様乱数（最大値Init Weight Range）で初期化 */
void init_weight_by_file(char *); /* 重みをファイルで初期化 */
void save_weights(char *);	/* 重みをファイルにセーブする */
void weight_set_to_0(float [][MaxNodes][MaxNodes]); /* 重みと同サイズの配列の初期値を０に設定 */

/* データ読み込み関連の関数 */
void prepare_run(int ,char *[]); /* パラメータファイル読み込む */
void get_params();		/* パラメータファイルよりパラメータ読み込み */
void getintpar(FILE *, char *,int *); /* 整数のパラメータ読み込み */
void getfltpar(FILE *, char *,float *); /* 実数のパラメータ読み込み */
void getcharpar(FILE *, char *,char *);	/* 文字列のパラメータ読み込み */
void get_id_file(char *);	/* id読み込み。data_name[][],datafile_id[],par.n_datafile設定。 */
void read_in_out_data(int ,char *); /* 入力データと教師データ（in[],out[]）を読み込む */
void dainyuu_input_data(float [], int , char []);
void error1(char *);		/* エラーメッセージを表示し、終了する */

Parstruct par;			/* パラメータ */

float	w[MaxLayers][MaxNodes][MaxNodes]; /* 重み[下の層番号−１][上のニューロン番号][下のニューロン番号] */
float	dw[MaxLayers][MaxNodes][MaxNodes]; /* 重み更新量[下の層番号−１][上のニューロン番号][下のニューロン番号] */
float	y[MaxLayers][MaxNodes];	/* 各層のニューロンの出力[下の層番号−１][ニューロン番号] */
float	t[MaxNodes];		/* 出力層への教師データ[ニューロン番号] */
float	d[MaxLayers][MaxNodes];	/* 各層への誤差信号[下の層番号−１][ニューロン番号] */
int 	data_id[MaxDataNum];	/* 各入力データのid番号[データ番号] */
float	in[MaxDataNum][MaxNodes]; /* 全入力データ[データ番号][ニューロン番号] */
float	out[MaxDataNum][MaxNodes]; /* 出力層への全教師データ[データ番号][ニューロン番号] */
int	num_of_teach_patterns; /* 学習データの総数 */
int	nodes_of_layer[MaxLayers]; /* 各層のニューロン数 */
int	nodes_from_weight[MaxLayers]; /* 重みデータファイルに記述された各層のニューロン数 */
int	nodes_from_tch[MaxLayers]; /* 学習用データファイルに記述された各層のニューロン数（かくれニューロン数以外） */
int	pattern_order[MaxDataNum*2]; /* パターンの学習順序 */

char	datafilelistname[200];	/* 学習用データファイル名のリストを含むファイル */
char	testfilelistname[200];	/* テスト用データファイル名のリストを含むファイル */
char	datafile_dir[200];	/* データファイルのあるディレクトリ名 */
char	datafile_name[MaxDataFileNum][MaxDataNameLength]; /* データファイル名 */
char	weightfilename[200];	/* 重み初期化用の重みを含むファイル名 */


int main(int argc,char *argv[])
{	
  int i;
  char read_file[200];
  int dfile;			/* データファイル番号 */
    
  get_params();			/* パラメータ読み込み */

  srand(SEED);			/* 乱数の種初期化 */

  par.o_layer=par.layers-1;	/* 出力層の層番号設定(0->入力層，出力層->par.layers-1) */

  if(par.learn_or_test == LEARN)
    get_id_file(datafilelistname); /* 学習用データファイル名のリスト読み込み。datafile_name[][],par.n_datafile設定。 */
  else
    get_id_file(testfilelistname); /* テスト用 */
  
  printf("Total Datafile = %d\n",par.n_datafile);
  if(par.n_datafile <= 0)
    error1("No Datafile in idlist");

  num_of_teach_patterns=0;
  for(dfile=0;dfile<par.n_datafile;dfile++)
  {
    sprintf(read_file,"%s/%s.tch",datafile_dir,datafile_name[dfile]);
    read_in_out_data(dfile,read_file);	/* 学習データ（in[],out[]）を読み込む */
  }
	
  set_nodes_of_layer();		/* 各層のニューロン数nodes_of_layer[]設定 */

  init_pattern_order();	        /* パターンの学習順序を初期化 */

  if((par.learn_or_test == LEARN) &&(par.weight_init_mode == 0))
    init_weight_by_ransuu();	/* 重みを一様乱数（最大値Weight Init Range）で初期化 */
  else
  {
    init_weight_by_file(weightfilename); /* 重みをファイルで初期化 */
  }

  if(par.learn_or_test == LEARN) /* 学習 */
  {
    int data;
    float calced_error;		/* 2乗平均誤差 */
    int pat_num;		/* 学習するデータ番号 */
    int learning_per_pat=0;	/* 学習回数 */
    float minimum_error=1.0;	/* 2乗平均誤差の最小値 */

    weight_set_to_0(dw);	/* 重みと同サイズの配列の初期値を０に設定 */
    while((minimum_error > par.error_conv) && (learning_per_pat < par.max_cycles))
    {

      randomize_pattern_order(); /* pattern_order[]をランダマイズ */
      
      for(data=0;data<num_of_teach_patterns;data++)
      {
	pat_num=pattern_order[data];
	for(i=0;i<nodes_of_layer[0];i++) /* 入力データ設定 */
	  y[0][i]=in[pat_num][i];
	calc_feedforward();	/* 各層のニューロンの出力計算 */
		    
	for(i=0;i<nodes_of_layer[par.o_layer];i++) /* 教師データ設定 */
	  t[i]=out[pat_num][i];

	calc_dw();		/* 重み更新量計算 */
	update_w();		/* 重み更新 */
      }		

      learning_per_pat++;	/* 学習回数加算 */

      calced_error=calc_error(); /* ２乗平均誤差計算 */
      if(calced_error < minimum_error)
      {
	char cyc[6];

	minimum_error=calced_error;
	save_weights("");	/* bptest.wout(最終的に誤差最小の重み)を上書き */

	sprintf(cyc,"%d",learning_per_pat);
	save_weights(cyc);	/* bptest.wout(学習回数)を出力(その回数での重み) */
      }


      printf("cycle=%d error=%g minimum=%g\n",learning_per_pat,calced_error,minimum_error);
    }

    if(minimum_error > par.error_conv)
      printf("Learning failed. Cycle=%d Error=%g\n",learning_per_pat,minimum_error);
    else
      printf("Learning succeeded. Cycle=%d Error=%g\n",learning_per_pat,minimum_error);
  }
  else				/* テスト */
  {
    int data;
    int pat_num;		/* テストするデータ番号 */
    int ans_number;
    float max_y;
    int n_correct=0;		/* 正解数 */
    
    for(data=0;data<num_of_teach_patterns;data++)
    {
      pat_num=pattern_order[data];
      for(i=0;i<nodes_of_layer[0];i++) /* 入力データ設定 */
	y[0][i]=in[pat_num][i];
      calc_feedforward();	/* 各層のニューロンの出力計算 */

      max_y=-1.0;
      ans_number=-1;
      for(i=0;i<nodes_of_layer[par.o_layer];i++) /* 出力最大ニューロン算出 */
	if(y[par.o_layer][i] > max_y)
	{
	  ans_number=i;
	  max_y=y[par.o_layer][i];
	}
      
      if(out[pat_num][ans_number] > 0.5) /* 正解の場合 */
	n_correct++;
      else
      {
	//	printf("id=%d -> %d\n",data_id[pat_num],ans_number);
      }
    }		
    printf("Correct rate= %6.2f(%%) (%d/%d)\n",(float)n_correct/(float)num_of_teach_patterns*100.0,n_correct,num_of_teach_patterns);
    
    printf("error=%g\n",calc_error()); /* 2乗平均誤差算出 */
  }
  
  return 1;
}

/* 各層のニューロンの出力計算 */
void calc_feedforward()
{
  int layer,layer_d;
  int i,j;
  float u;
    
  for(layer=1;layer<par.layers;layer++)	/* 下位層から */
  {
    layer_d=layer-1;		/* 一つ下の層番号 */
    for(i=0;i<nodes_of_layer[layer];i++)
    {
      u=0.0;
      for(j=0;j<nodes_of_layer[layer_d];j++) /* 総入力計算 */
	/*(A)*/
	u += 

      if(u < -90.0)		/* exp()のエラーを避けるため総入力の絶対値を90未満とする */
	u=-90.0;
      else if(u > 90.0)
	u=90.0;

      /*(B)*/
      y[layer][i]=
    }
  }	
}

/* 重み更新量計算 */
void calc_dw()
{
  int layer,layer_u;
  int i,j;
  float sum,temp;
    
  for(i=0;i<nodes_of_layer[par.o_layer];i++) /* 出力層のエラー(誤差)計算 */
    d[par.o_layer][i]=t[i]-y[par.o_layer][i];

  for(layer=par.layers-2;layer>=0;layer--) /* 出力層に近い層(上位層)への重みから順番に計算 */
  {
    layer_u=layer+1;		/* 上位層の層番号 */
    for(j=0;j<nodes_of_layer[layer];j++)
    {
      sum=0.0;
      for(i=0;i<nodes_of_layer[layer_u];i++)
      {
	/*(C)*/ /* 上位層が出力層以外の場合でも算出できるようにする必要がある */
	temp=

	dw[layer][i][j]=par.epsilon*temp*y[layer][j]+par.alpha*dw[layer][i][j];
	sum += temp*w[layer][i][j];
      }
      d[layer][j]=sum;		/* 次の層のニューロンでの誤差 */
    }
  }
}

/* 重み更新 */
void update_w()
{
  int layer,j,i;
    
  for(layer=0;layer<par.o_layer;layer++)
    for(i=0;i<=nodes_of_layer[layer+1];i++)
      for(j=0;j<nodes_of_layer[layer];j++)
	w[layer][i][j] += dw[layer][i][j];
}

/* ２乗平均誤差計算 */
float calc_error() 
{
  int data;
  int i;
  float sum,e;
  int pat_num;
    
  sum=0.0;
  for(data=0;data<num_of_teach_patterns;data++)
  {
    pat_num=pattern_order[data];
    for(i=0;i<nodes_of_layer[0];i++) /* 入力データ設定 */
      y[0][i]=in[pat_num][i];
    calc_feedforward();		/* 各層のニューロンの出力計算 */
	
    for(i=0;i<nodes_of_layer[par.o_layer];i++) /* そのパターンの誤差計算 */
    {
      e=y[par.o_layer][i]-out[pat_num][i];
      sum += e*e;
    }	    
  }		
  sum /= (float)(num_of_teach_patterns*2.0);
    
  return sum;
}



#define CountLimitBairitufile 2000

/* id読み込み。data_name[][],par.n_datafile設定。 */
void get_id_file(char *str)	
{
  FILE *wtfile;
  int	y;			/* データ読み込みのための変数 */
  int end_flag;
  char tfilename[TxtLineLen];
  char inlinetit[TxtLineLen];
  char temp_name[MaxDataNameLength];
  char read_char[MaxDataNameLength];
  int count_limit;
    
  sprintf(tfilename,"%s",str);
  if ((wtfile=fopen(tfilename,"r"))!=NULL) 
  {
    count_limit=0;
    sprintf(read_char," ");
    while(strcmp(read_char,"$BEGIN") != 0) /* $BEGINを探す */
    {
      fgets(inlinetit,TxtLineLen,wtfile);
      sscanf(inlinetit,"%s",read_char);
      count_limit++;
      if(count_limit > CountLimitBairitufile)
	error1("$BEGIN doesn't exist in idlist");
    }

    fgets(inlinetit,TxtLineLen,wtfile); /* 入力データファイルのあるディレクトリ名を読み込む */
    sscanf(inlinetit,"%s",datafile_dir);

    y=0;
    end_flag=NO;
    count_limit=0;
    while ((end_flag == NO)&&(fgets(inlinetit,TxtLineLen,wtfile)!=NULL)) 
    {              
      count_limit++;
      if(count_limit > CountLimitBairitufile)
	error1("$END doesn't exist in idlist");

      if(y >= MaxDataFileNum)
	error1("Please increase MaxDataFileNum in bp.h");

      sscanf(inlinetit,"%s",read_char);
      if(strcmp(read_char,"$END") != 0) /* $ENDまでデータ読み込み */
      {
	if(inlinetit[0] != '#')
	{
	  sscanf(inlinetit,"%s",temp_name); 
	  strcpy(datafile_name[y],temp_name); /* データ名 */
#ifdef DEBUG
	  printf("%s\n",datafile_name[y]);
#endif
	  y++;
	}
      }
      else
	end_flag=OK;
    }	
    par.n_datafile=y;		/* データファイルの総数 */

    fclose(wtfile);
  }
  else
    error1("No id file exist");
}

/* 入力データと教師データ（in[],out[]）を読み込む */
void read_in_out_data(int kaisuu,char *rfile) 
     /*int kaisuu;*/
     /*char *rfile;*/	/* *.tch(pathをつけない) */
{
  char inlinetit[TxtLineLen];	/* fgetsで読み込む文字配列 */
  int n_data;			/* データ読み込みのための変数 */
  int n_data_onefile;
  int nodes[MaxLayers];
  int i;
  int ans_number;
  FILE *fp_tmp_wt ;

  if((fp_tmp_wt = fopen(rfile,"r")) == NULL) {
    fprintf(stderr, "ERROR: can't open tch file.") ;
    exit(-1) ;
  }

  fgets (inlinetit,TxtLineLen,fp_tmp_wt);	
  sscanf(inlinetit,"%d",&n_data_onefile);
  if(n_data_onefile <= 0)
    error1("Wrong number of patterns in tch file");	

  fgets (inlinetit,TxtLineLen,fp_tmp_wt);	
  sscanf(inlinetit,"%d",&nodes[0]);
  if(nodes[0] <= 0)
    error1("Wrong number of input neurons in tch file");	
  if(kaisuu == 0)
    nodes_from_tch[0]=nodes[0];
  else if(nodes[0] != nodes_from_tch[0])
    error1("Input dimensions are different among files.");

  fgets (inlinetit,TxtLineLen,fp_tmp_wt);	
  sscanf(inlinetit,"%d",&nodes[par.layers-1]);
  if(nodes[par.layers-1] <= 0)
    error1("Wrong number of output neurons in tch file");	
  if(kaisuu == 0)
    nodes_from_tch[par.layers-1]=nodes[par.layers-1];
  else if(nodes[par.layers-1] != nodes_from_tch[par.layers-1])
    error1("Output dimensions are different among files.");
	
  for(n_data=0;n_data<n_data_onefile;n_data++) {
    if(fgets(inlinetit,TxtLineLen,fp_tmp_wt) == NULL)
      error1("Not enough data in tch file");
    sscanf(inlinetit,"%d",&data_id[num_of_teach_patterns]);
    
    if(fgets(inlinetit,TxtLineLen,fp_tmp_wt) == NULL)
      error1("No input data line in tch file");
    else
    {
      dainyuu_input_data(in[num_of_teach_patterns],nodes_from_tch[0],inlinetit);
    }
	    
    if(fgets(inlinetit,TxtLineLen,fp_tmp_wt) == NULL)
      error1("No output data line in tch file");
    else {
      sscanf(inlinetit,"%d",&ans_number);
      for(i = 0; i < nodes_from_tch[par.layers-1]; i++) {
	if(i == ans_number)
	  out[num_of_teach_patterns][i] = 1.0 ;
	else
	  out[num_of_teach_patterns][i] = 0.0 ;
      }
    }
    num_of_teach_patterns++;
  }
  
  fclose(fp_tmp_wt) ;
}    

/* 数値データに変換 */
void dainyuu_input_data(float data[], int dim, char linebuf[])
{
  int n,n1;
  int np=0;
  char char1[256];
  float wtempcon;
  
  for(n=0;n<dim;n++)
  {				/* 空白でない次の文字を探す */
    while((linebuf[np] == ' ') || (linebuf[np] == '\t'))
    {
      np++;
      if(np >= MaxNodes*30)
	error1("Too long line in tch file (Not enough dimension?)");
    }
	      
    n1=0;
    while((linebuf[np] != ' ') && (linebuf[np] != '\t') && (linebuf[np] != '\n'))
    {
      char1[n1]=linebuf[np];
      np++;
      n1++;
      if(n1 >= 256)
	error1("Too long number in tch file");
    }
    
    char1[n1]='\0';
    if(sscanf(char1,"%f",&wtempcon) <= 0)
      error1("Irregal data in tch file");
    else
      data[n]=wtempcon;
  }
}

/* 各層のニューロン数nodes_of_layerを設定 */
void set_nodes_of_layer()	
{
  int i;

  nodes_of_layer[0]=nodes_from_tch[0];

  nodes_of_layer[par.o_layer]=nodes_from_tch[par.o_layer];
  if(par.layers >= 3)
    nodes_of_layer[1]=par.hidden_nodes1;

  for(i=0;i<par.layers;i++)	/* num_of_nodes[]のチェック */
    if(nodes_of_layer[i] <= 0)
    {
      printf("layer=%d: Nodes should be more than 0",i);
      error1(" ");
    }
    else if(nodes_of_layer[i] >= MaxNodes)
    {
      printf("layer=%d: Nodes should be less than %d",i,MaxNodes);
      error1(" ");
    }

#ifdef DEBUG
  printf("Layers=%d\n",par.layers);
  for(i=0;i<par.layers;i++)
    printf("layer=%d nodes=%d\n",i+1,nodes_of_layer[i]);
#endif 
}

/* パターンの学習順序を初期化 */
void init_pattern_order()
{
  int i;
    
  for(i=0;i<num_of_teach_patterns;i++)
    pattern_order[i]=i;
}

/* パターンの学習順序pattern_order[]をランダマイズ */
void randomize_pattern_order()	
{
  int i,temp;
  int c1,c2;
    
  for(i=0;i<num_of_teach_patterns/2;i++)
  {
    c1=rnum(num_of_teach_patterns);
    c2=rnum(num_of_teach_patterns);
    temp=pattern_order[c1];
    pattern_order[c1]=pattern_order[c2];
    pattern_order[c2]=temp;
  }
}

/* エラーメッセージを表示し、終了する */
void error1(char *errtxt)
{
  fprintf(stderr,"Error: %s. Aborting.\n",errtxt);
  fflush(stderr);
  exit(1);
}

/* ０からrange-1の間の一様乱数を返す */
int rnum(int range)
     /* range */	/* 乱数の幅 */
{
  return rand()%range;
}

/* 重みを一様乱数（最大値Init Weight Range）で初期化 */
void init_weight_by_ransuu()
{
  int layer,node1,node2;
    
  for(layer=0;layer<par.layers-1;layer++)
    for(node2=0;node2<nodes_of_layer[layer+1];node2++)
      for(node1=0;node1<nodes_of_layer[layer];node1++)
	w[layer][node2][node1]=(float)(rnum(2001)-1000)/1000.0*par.weight_init_range;

#ifdef DEBUG
  for(layer=0;layer<par.layers-1;layer++)
  {
    printf("layer=%d -> %d\n",layer+1,layer+2);
    for(node2=0;node2<nodes_of_layer[layer+1];node2++)
    {
      printf("node_up=%d num_nodes_down=%d ",node2,nodes_of_layer[layer]);
      for(node1=0;node1<nodes_of_layer[layer];node1++)
	printf("%g ",w[layer][node2][node1]);
      printf("\n");
    }
  }
#endif    
}

/* 重みをファイルで初期化 */
void init_weight_by_file(char *wfile)
{
  char	inlinetit[TxtLineLen];	/* fgetsで読み込む文字配列 */
  FILE *wtfile;			/* 重みファイル */
  char scstr[20];		/* データ読み込みの書式 */
  int scpos,d,n,dum,i;		/* データ読み込みのための変数 */
  int nod,lay,chars;
  float wtempcon;
    
  if ((wtfile=fopen(wfile,"r"))!=NULL) 
  {
    d=n=0;
    fgets (inlinetit,TxtLineLen,wtfile);	
    sscanf(inlinetit,"%d",&dum);
    if (dum != par.layers)
      error1("Wrong number of layers in weight_file");	
    for(i=0;i<par.layers;i++)
    {
      fgets (inlinetit,TxtLineLen,wtfile);	
      sscanf(inlinetit,"%d",&dum);
      if (dum != nodes_of_layer[i])
	error1("Wrong number of nodes in weight_file");	
    }
	
    for(i=par.o_layer-1;i>=0;i--)
    {
      for(n=0;n<nodes_of_layer[i+1];n++)
      {
	if(fgets(inlinetit,TxtLineLen,wtfile) == NULL)
	  error1("Not enough data in weight_file");
	if(sscanf(inlinetit,"layer=%d node=%d",&lay,&nod) <= 0)
	  error1("Wrong data in weight_file");
	if(lay != i+1)
	  error1("Wrong data in weight_file");
	if(nod != n)
	  error1("Wrong data in weight_file");
	chars=0;
	if(fgets(inlinetit,TxtLineLen,wtfile) == NULL)
	  error1("Not enough data in weight_file");
	sprintf(scstr,"%%f%%n");
	for(d=0;d<nodes_of_layer[i];d++)
	{
	  if(sscanf(inlinetit,scstr,&wtempcon,&scpos) <= 0)
	    error1("Wrong data in weight_file");
	  w[i][n][d]=wtempcon;
	  chars++;
	  if((chars%10 == 0) && (d != nodes_of_layer[i]-1))
	  {
	    chars=0;
	    if(fgets(inlinetit,TxtLineLen,wtfile) == NULL)
	      error1("Not enough data in weight_file");
	    sprintf(scstr,"%%f%%n");
	  }
	  else
	    sprintf(scstr,"%%*%dc%%f%%n",scpos);
	}
      }
    }
    fclose(wtfile);
  }
  else
    error1("No weight file exist");
}

/* 重みと同サイズの配列の初期値を０に設定 */
void weight_set_to_0(float ww[][MaxNodes][MaxNodes])
{
  int layer,node1,node2;
    
  for(layer=0;layer<par.o_layer;layer++)
    for(node2=0;node2<=nodes_of_layer[layer+1];node2++)
      for(node1=0;node1<nodes_of_layer[layer];node1++)
	ww[layer][node2][node1]=0.0;
}

/* 重みをファイルにセーブする */
void save_weights(char *extens)	
     /*  char *extens; */	/* ファイル名の最後につける名前 */
{
  FILE *wtfile;			/* 重みファイル */
  char tfilename[TxtLineLen];
  int i,chars,n,d;		/* 重み書き込み用変数 */
    
  sprintf(tfilename,"bp%s.wout%s",ident,extens);
  if ((wtfile=fopen(tfilename,"w"))==NULL) error1("Can't write weight-file");
  fprintf(wtfile,"%d\n",par.layers);
  for(i=0;i<par.layers;i++)
    fprintf(wtfile,"%d\n",nodes_of_layer[i]);
  for(i=par.o_layer-1;i>=0;i--)
    for (n=0;n<nodes_of_layer[i+1];n++)
    {
      fprintf(wtfile,"layer=%d node=%d\n",i+1,n);
      chars=0;
      for (d=0;d<nodes_of_layer[i];d++)
      {
	fprintf(wtfile,"%12f ",w[i][n][d]);
	chars++;
	if (chars==10)
	{
	  chars=0;
	  fprintf(wtfile,"\n");
	}
      }
      if(nodes_of_layer[i]%10 != 0)
	fprintf(wtfile,"\n");
    }
  fclose(wtfile);
}

/* パラメータファイルよりパラメータ読み込み */
void get_params()
{
  char tfilename[TxtLineLen];
  FILE *parfile;
  
  sprintf(tfilename,"bp%s.par",ident);
  if ((parfile=fopen(tfilename,"r"))==NULL)
  {
    printf("file=%s\n",tfilename);
    error1("No parameter file");
  }

  getintpar(parfile, "Learn or Test",&par.learn_or_test); /* 0:learn 1:test */
  getintpar(parfile, "Layers",&par.layers);
  getintpar(parfile, "Hidden Nodes1",&par.hidden_nodes1);
  getfltpar(parfile, "Threshold Error",&par.error_conv);
  getintpar(parfile, "Maximum Cycles",&par.max_cycles);
  getfltpar(parfile, "Learning Rate",&par.epsilon);
  getfltpar(parfile, "Alpha Rate",&par.alpha);
  getintpar(parfile, "Weight Init Mode",&par.weight_init_mode); /* 0:init by random  1:init by file */
  getfltpar(parfile, "Weight Init Range",&par.weight_init_range);
  getcharpar(parfile, "Weight for Init",weightfilename);
  getcharpar(parfile, "DatafileList for Learn",datafilelistname);
  getcharpar(parfile, "DatafileList for Test",testfilelistname);
  
  fclose(parfile);	  

  if((par.layers <= 1) || (par.layers >= 4))
    error1("2 <= Layers <= 3 in par_file");
}

/* 整数のパラメータ読み込み */
void getintpar(FILE *parfile, char *string,int *var)
     /*  char string[40]; */	/* パラメータファイルにおけるパラメータの前書き */
     /*   int *var; */		/* パラメータ */
{
  char	inlinetit[TxtLineLen];	/* fgetsで読み込む文字配列 */
  int	iparinc,iparend;
  char *succ;			/* 関数fgets()の出力を代入する変数 */
  char template[40];		/* パラメータを読み込む書式 */
  int no_of_vars;		/* そのパラメータの行に書かれた数値の個数 */
    
  do succ=fgets(inlinetit,TxtLineLen,parfile); 
  while ((inlinetit[0]=='*'||inlinetit[0]==' '||inlinetit[0]=='\n')&&succ!=NULL);
  sprintf(template,"%s: %%d %%d %%d",string);
  no_of_vars=sscanf(inlinetit,template,var,&iparinc,&iparend);
  switch (no_of_vars)
  {
  case 1: break;
  default:
    fprintf(stderr,"Line: %s\n",string);
    error1("ReadParams: Parameters missing");
    break;
  }
}

/* 実数のパラメータ読み込み */
void getfltpar(FILE *parfile,char *string,float *var)	
     /*  char string[40]; */	/* パラメータファイルにおけるパラメータの前書き */
     /*  float *var; */		/* パラメータ */
{
  char	inlinetit[TxtLineLen];	/* fgetsで読み込む文字配列 */
  float fparinc,fparend;
  char *succ;			/* 関数fgets()の出力に代入する変数 */
  char template[40];		/* パラメータを読み込む書式 */
  int no_of_vars;		/* そのパラメータの行に書かれた数値の個数 */
    
  do succ=fgets(inlinetit,TxtLineLen,parfile);
  while ((inlinetit[0]=='*'||inlinetit[0]==' '||inlinetit[0]=='\n')&&succ!=NULL);
  sprintf(template,"%s: %%g %%g %%g",string);
  no_of_vars=sscanf(inlinetit,template,var,&fparinc,&fparend);
  switch (no_of_vars)
  {
  case 1: break;
  default:
    fprintf(stderr,"Line: %s\n",string);
    error1("ReadParams: Parameters missing");
    break;
  }
}

/* 文字列のパラメータ読み込み */
void getcharpar(FILE *parfile,char *string,char *var)	
     /*  char string[100]; */	/* パラメータファイルにおけるパラメータの前書き */
     /*  char *var; */		/* パラメータ */
{
  char	inlinetit[TxtLineLen];	/* fgetsで読み込む文字配列 */
  char *succ;			/* 関数fgets()の出力を代入する変数 */
  char template[100];		/* パラメータを読み込む書式 */
  int no_of_vars;		/* そのパラメータの行に書かれた数値の個数 */
    
  do succ=fgets(inlinetit,TxtLineLen,parfile); 
  while ((inlinetit[0]=='*'||inlinetit[0]==' '||inlinetit[0]=='\n')&&succ!=NULL);
  sprintf(template,"%s: %%s",string);
  no_of_vars=sscanf(inlinetit,template,var);
  switch (no_of_vars)
  {
  case 1: break;
  default:
    fprintf(stderr,"Line: %s\n",string);
    error1("ReadParams: Parameters wrong");
    break;
  }
}

