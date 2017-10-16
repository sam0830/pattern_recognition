/**************************************************/
/*****            maeshori.c                    ***/
/*****         切出し画像の前処理               ***/
/*****    2値化，ノイズ除去，正規化             ***/
/*****    Usage: a.out ID                       ***/
/*****    Function: 文字画像を前処理して出力    ***/
/*****    Input:  切出し文字画像                ***/
/*****    Output: 正規化文字画像                ***/
/*****                                          ***/
/**************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char UCHAR;

#define GYOU_MOJI 100		/* 切り出す画像の行数 */
#define RETU_MOJI GYOU_MOJI	/* 切り出す画像の列数 */
#define N_MOJI 10		/* 文字種の総数 */
#define N_SAMPLE 6		/* 1文字あたりのデータ数 */
#define THRESH_BIN 150		/* 2値化する閾値 */
#define THRESH_MENSEKI 52	/* ノイズ除去のため面積がこれ以下の領域を削除 */
#define MaxLabel 500		/* ラベル数の最大値(想定される値より大きめに設定) */
#define SIZE_RATIO 0.90		/* 文字を正規化する際の行(列)数に対するサイズ比率 */

#define BLACK 0			/* 黒の画素値 */
#define WHITE 255		/* 白の画素値 */
#define NOLABEL -1		/* ラベルのない所の値 */
#define RENKETU8 8		/* 8連結でラベリング */
#define RENKETU4 4		/* 4連結でラベリング */
#define OK 1
#define NO -1

void read_cut_pgm(UCHAR [][RETU_MOJI], char *,int *,int *);
void write_cut_pgm(UCHAR [][RETU_MOJI],char *,int ,int );
void binarize_image(UCHAR [][RETU_MOJI], UCHAR [][RETU_MOJI], int, int, int);
int labeling(int [][RETU_MOJI],int ,UCHAR [][RETU_MOJI],int ,int ,int );
void add_to_label_matrix(int , int , unsigned short [][MaxLabel], int [], unsigned short [][MaxLabel]);
int remove_small_labelled_area(int [][RETU_MOJI],int ,int ,int ,int);
void make_bin_image_from_label_image(UCHAR [][RETU_MOJI],int [][RETU_MOJI],int ,int ,UCHAR );
void seikika(UCHAR [][RETU_MOJI],int ,int , float , UCHAR );
void error1(char *);


int main(int argc,char *argv[]){
    int i,j;
    int in_retu,in_gyou;
    UCHAR org[GYOU_MOJI][RETU_MOJI]; /* 入力画像 */
    UCHAR res[GYOU_MOJI][RETU_MOJI]; /* 出力画像 */
    char in_fname[200];		/* 入力画像のファイル名 */
    char out_fname[200];		/* 出力画像のファイル名 */
    int num_label;		/* ラベル数 */
    int label_data[GYOU_MOJI][RETU_MOJI]; /* ラベル画像 */
    char MY_ID[10];		/* 学籍番号 */

    if(argc != 2)
        error1("a.out ID");
    else {
        strcpy(MY_ID,argv[1]);
    }

    for(i=0;i<N_MOJI;i++)		/* すべての数字について処理 */
        for(j=0;j<N_SAMPLE;j++) {
            sprintf(in_fname,"./Org/%s/%s-%1d-%1d.pgm",MY_ID,MY_ID,i,j); /* 入力画像ファイル名 */
            read_cut_pgm(org, in_fname, &in_retu, &in_gyou);  /* 入力画像読み込み */
            if((in_retu != RETU_MOJI) || (in_gyou != GYOU_MOJI))
	            error1("Dimensions are wrong.");

				/* 2値化 */
            binarize_image(res, org, in_retu, in_gyou, THRESH_BIN);

				/* ラベリング */
            num_label=labeling(label_data, BLACK, res, in_gyou, in_retu, RENKETU4);

				/* ラベル数を表示(不要ならコメントアウトする) */
            printf("suuji=%d no=%d num_label %d -> ",i,j,num_label);

                                /* ノイズのある可能性がある場合のみノイズ除去処理 */
            if(num_label > 1)		/* 面積が閾値以下のラベルを削除．削除後のラベル数を返す． */
	            num_label=remove_small_labelled_area(label_data, in_gyou, in_retu, num_label, THRESH_MENSEKI);

            printf("%d\n",num_label);	/* (不要ならコメントアウトする) */

            if(num_label <= 0) {	/* 不正な画像のチェック */
	            printf("Result of %s has no label. Please check.\n", in_fname);
	            continue;
            }
				/* ラベルのあるところと無いところの2値化画像を作成 */
				/* ラベル有りはBLACK, 無しはWHITE */
            make_bin_image_from_label_image(res, label_data, in_gyou, in_retu, (UCHAR)BLACK);

				/* 正規化(BLACK画素の外接長方形の中心を画像の中心とする) */
      /* 縦あるいは横の最大サイズが行数あるいは列数*SIZE_RATIOとなるように等方的に拡大(縮小) */
            seikika(res, in_gyou, in_retu, (float)SIZE_RATIO, (UCHAR)BLACK);

            sprintf(out_fname,"./Mae/%s/%smae-%1d-%1d.pgm",MY_ID,MY_ID,i,j); /* 出力画像ファイル名 */
            write_cut_pgm(res, out_fname, in_retu, in_gyou);  /* 出力画像書き込み */
        }

    return 0;
}

/* 画像の2値化 */
void binarize_image(UCHAR res[][RETU_MOJI], UCHAR org[][RETU_MOJI], int n_retu, int n_gyou, int thresh) {
    int g,r;

    for(g=0;g<n_gyou;g++)
        for(r=0;r<n_retu;r++)
            if(org[g][r] >= thresh)
	            res[g][r]=WHITE;
            else
	            res[g][r]=BLACK;
    }

/***********************************************************************
 *
 * remove_small_labelled_area -- 総面積がthresh未満のラベル領域を削除する
 *
 * Syntax       : int remove_small_labelled_area(int l_data[][RETU_MOJI], int dim_gyou,
 *			int dim_retu, int num_label_old, int thresh)
 *      Input   : int l_data[][RETU_MOJI]; ラベル画像
 *                int dim_gyou          ; 行数
 *                int dim_retu          ; 列数
 *                int num_label_old     ; 削除前のラベル数
 *                int thresh            ; 面積の閾値
 *      Output  : int l_data[][RETU_MOJI]; 変換後のラベル画像
 * Returns      : 削除後のラベル数
 *
 ***********************************************************************/
int remove_small_labelled_area(int l_data[][RETU_MOJI],int dim_gyou,int dim_retu,int num_label_old,int thresh) {
    int i,g,r;
    int num_label_new;
    int label_menseki[MaxLabel]; /* 各ラベルの面積 */
    int new_label[MaxLabel];	/* 削除後の新ラベル番号[旧ラベル番号] */

    for(i=0;i<num_label_old;i++) /* 面積の初期値を0に設定 */
        label_menseki[i]=0;

    for(g=0;g<dim_gyou;g++)	/* 面積算出 */
        for(r=0;r<dim_retu;r++)
            if(l_data[g][r] >= 0)
                label_menseki[l_data[g][r]]++;

    num_label_new=0;		/* ラベルの変換表 new_label[] を作成する */
    for(i=0;i<num_label_old;i++) {/* 面積が閾値未満のラベルの新ラベルをNOLABELとすることで削除*/
        if(label_menseki[i] >= thresh) {
            new_label[i]=num_label_new;
            num_label_new++;
        }
        else {
            new_label[i]=NOLABEL; /* 面積が閾値未満のラベルの新ラベルはNOLABEL*/
        }
    }

    for(g=0;g<dim_gyou;g++)     /* labelをつけかえる */
        for(r=0;r<dim_retu;r++)
            if(l_data[g][r] >= 0)
                l_data[g][r]=new_label[l_data[g][r]];

    return(num_label_new);
}


/* ラベルのあるところと無いところの2値化画像を作成 */
/* ラベル有りはval, 無しはWHITE-val */
void make_bin_image_from_label_image(UCHAR res[][RETU_MOJI],int l_data[][RETU_MOJI],int dim_gyou,int dim_retu,UCHAR val) {
    int g,r;

    for(g=0;g<dim_gyou;g++)
        for(r=0;r<dim_retu;r++)
            if(l_data[g][r]==NOLABEL)
	            res[g][r]=(UCHAR)(WHITE-val);
            else
	            res[g][r]=val;
}

/* 正規化(BLACK画素の外接長方形の中心を画像の中心とする) */
/* 縦あるいは横の最大サイズが行数あるいは列数のratio_target倍となるように等方的に拡大(縮小) */
void seikika(UCHAR res[][RETU_MOJI],int dim_gyou,int dim_retu, float ratio_target, UCHAR val) {
    int g,r;
    UCHAR tmp[GYOU_MOJI][RETU_MOJI]; /* 一時画像メモリ */
    int min_g=dim_gyou-1;
    int max_g=0;
    int min_r=dim_retu-1;
    int max_r=0;
    float cent_g,cent_r;		/* 元画像の外接長方形の中心座標 */
    float new_cent_g,new_cent_r;	/* 正規化画像の外接長方形の中心座標 */
    float ratio_g,ratio_r;	/* 元画像の文字の行(列)数/画像の行(列)数 */
    float ratio_change;		/* 最終的な文字の拡大(縮小)率 */
    int inv_g,inv_r;		/* 逆変換座標 */

    for(g=0;g<dim_gyou;g++)	/* 外接長方形算出 */
        for(r=0;r<dim_retu;r++)
            if(res[g][r] == val) {
	            if(g < min_g)
	                min_g=g;
	            if(g > max_g)
	                max_g=g;
	            if(r < min_r)
	                min_r=r;
	            if(r > max_r)
	                max_r=r;
            }

    cent_g=(float)(min_g+max_g)/2.0; /* 文字の中心座標(正規化前) */
    cent_r=(float)(min_r+max_r)/2.0;

    new_cent_g=(float)(GYOU_MOJI-1)/2.0; /* 文字の中心座標(正規化後) */
    new_cent_r=(float)(RETU_MOJI-1)/2.0;

    ratio_g=(float)(max_g-min_g)/(float)(GYOU_MOJI-1);
    ratio_r=(float)(max_r-min_r)/(float)(RETU_MOJI-1);

    /******** (A)最終的な倍率(ratio_change)を決定する処理を入れる ***********/
    ratio_change = (ratio_g>ratio_r)?SIZE_RATIO/ratio_g:SIZE_RATIO/ratio_r; /*ratio_gとratio_rの大きいを基準にする*/\
				/* 正規化画像算出(逆変換座標算出，最近隣内挿法利用) */
				/* res[inv_g][inv_r]をもとにして正規化画像tmp[g][r]を作成する */
                                /* 逆変換座標算出，最近隣内挿法． */
				/* 領域外ならばWHITE-val(背景画素値)とする点に注意． */
				/******** (B)このループ内を完成させる *********/
    for(g=0;g<dim_gyou;g++)	{/* この行以外を変更して良い． */
        for(r=0;r<dim_retu;r++)	{
            inv_g = (g-new_cent_g)/ratio_change + cent_g + 0.50f;
            inv_r = (r-new_cent_r)/ratio_change + cent_r + 0.50f;
            /*領域外処理*/
            if(inv_g < min_g || inv_g > max_g) {
                tmp[g][r] = WHITE;
                continue;
            }
            if(inv_r < min_r || inv_r > max_r) {
                tmp[g][r] = WHITE;
                continue;
            }
            tmp[g][r] = res[inv_g][inv_r];
        }
    }

    for(g=0;g<dim_gyou;g++)	/* 元画像に代入 */
        for(r=0;r<dim_retu;r++)
            res[g][r]=tmp[g][r];
}




/* pgmフォーマットの画像ファイル作成 */
void write_cut_pgm(UCHAR data_buf[][RETU_MOJI],char *fname,int width,int height) {
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
        for(n=0;n<width;n++) {
            val[i++]=data_buf[m][n];
        }

    fwrite(val, sizeof(UCHAR), width*height, fp); /* ファイルへの書き込み */

    fclose(fp);

    free(val);
}


/* pgmフォーマットの画像ファイル読み込み */
void read_cut_pgm(UCHAR data_buf[][RETU_MOJI], char *fname,int *width,int *height) {
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
    while(1) {
        c = getc(fp) ;

        if(com_flg) {
            if(c == '\n')
	        com_flg-- ;
        } else {
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
  if((*height != GYOU_MOJI) || (*width != RETU_MOJI)) {
      fprintf(stderr, "read_ppm: ERROR: Dimension dosenot match.\n");
      exit(-1) ;
  }

  /* 画像データ部分読み込み */
  k=0;
  fread(val, sizeof(UCHAR), GYOU_MOJI*RETU_MOJI, fp);
  for( m = 0; m < *height; m++)
      for( n = 0; n < *width; n++) {
          data_buf[m][n] = val[k++];
      }

  /* ファイルを閉じる */
  fclose(fp) ;
}


/* エラー処理 */
void error1(char *message) {
    printf("%s\n",message);
    exit(1);
}


/***********************************************************************
 *
 * labeling -- 入力画像g_dataの値がlevelの画素を対象としてラベリングする
 *
 * Syntax       : int labeling(int l_data[][RETU_MOJI],int level,
 *                             UCHAR g_data[][RETU_MOJI],int dim_gyou,int dim_retu,
 *                             int renketu)
 *       Input  : int level             ; ラベリング対象となる画素の値
 *                UCHAR g_data[][]      ; 入力画像
 *                int dim_gyou          ; 行数
 *                int dim_retu          ; 列数
 *                int renketu           ; ラベリング時の連結情報
 *       Output : int l_data[][]        ; ラベリング結果の画像
 * Returns      : ラベル数
 *
 ***********************************************************************/
int labeling(int l_data[][RETU_MOJI],int level,UCHAR g_data[][RETU_MOJI],int dim_gyou,int dim_retu,int renketu) {
    int i,j,g,r;
    int r2;
    int gl;
    int num_label;
    int label_comb[2][RETU_MOJI*GYOU_MOJI/2];
    int num_label_comb;
    int r_start=0,r_end=0;
    int r_start_g=0,r_end_g=0;
    int cont_flag;
    int num_con_label;
    int num_con_label2;
    int con_label[RETU_MOJI/2];
    int con_label2[RETU_MOJI/2];
    int label_min;
    int last_label;
    int this_label;
    int num_label_group;
    int num_label_member[MaxLabel];
    int num_aite[MaxLabel];
    int new_label[MaxLabel];
    unsigned short label_matrix[MaxLabel][MaxLabel];
    unsigned short label_member[MaxLabel][MaxLabel];
    unsigned short aite[MaxLabel][MaxLabel];


    for(g=0;g<dim_gyou;g++)     /* 各画素のラベルの初期値を−１に設定 */
        for(r=0;r<dim_retu;r++)
            l_data[g][r]=NOLABEL;

    num_label=-1;               /* １行目のラベル設定 */

    if(g_data[0][0] == level) {
        num_label++;
        l_data[0][0]=num_label;
    }

    g=0;
    for(r=1;r<dim_retu;r++)
        if(g_data[g][r] == level) {
            if(g_data[g][r-1] != level)
                num_label++;
            l_data[g][r]=num_label;
        }
    num_label++;

    num_label_comb=0;
    for(g=1;g<dim_gyou;g++) {
        r=0;                    /* １行について、ラン（r_start<->r_end）を求め、処理する */
        cont_flag=NO;
        while(r < dim_retu) {
            if(cont_flag == NO) {
                if(g_data[g][r] == level) {
                    r_start_g=r;
                    cont_flag=OK;

                    if(renketu == RENKETU8) {/* ８連結の場合 */
                        if(r_start_g != 0)
                            r_start=r_start_g-1;
                        else
                            r_start=0;
                    }
                    else        /* ４連結の場合 */
                        r_start=r_start_g;
                }
            }
            else {
                if(g_data[g][r] != level) {
                    r_end_g=r-1;
                    cont_flag=NO;

                    if(renketu == RENKETU8) /* ８連結の場合 */
                        r_end=r;
                    else        /* ４連結の場合 */
                        r_end=r_end_g;
                }
                else if(r == dim_retu-1) {
                    r_end_g=r;
                    r_end=r;
                    cont_flag=NO;
                }

                if(cont_flag == NO) {/* OKからNOへ変化した場合、定義された１つのランについて処理する */
                    gl=g-1;     /* １行前の連結するランのラベルを求める */
                    num_con_label=0;
                    last_label=NOLABEL;
                    for(r2=r_start;r2<=r_end;r2++) {
                        this_label=l_data[gl][r2]; /* ポインタ使用により高速化可能 */
                        if(this_label != last_label) {
                            if(this_label >= 0) {
                                con_label[num_con_label]=this_label;
                                last_label=this_label;
                                num_con_label++;
                            }
                            else
                                last_label=NOLABEL;
                        }
                    }

                    if(num_con_label == 0) {/* 上と連結しないランの場合 */
                        for(r2=r_start_g;r2<=r_end_g;r2++)
                            l_data[g][r2]=num_label;
                        num_label++;
                    }
                    else {        /* 上と連結する場合 */
                        label_min=100000000;
                        num_con_label2=0; /* con_labelを重複しないように圧縮しcon_label2とする */
                        for(i=0;i<num_con_label;i++) {
                            if(con_label[i] >= 0) {/* 新しいlabelの場合 */
                                con_label2[num_con_label2]=con_label[i];
                                num_con_label2++;
                                for(j=i+1;j<num_con_label;j++)
                                    if(con_label[j] == con_label[i])
                                        con_label[j]=-1;

                                if(con_label[i] < label_min) /* 最小ラベルを求める */
                                    label_min=con_label[i];
                            }
                        }

                        for(r2=r_start_g;r2<=r_end_g;r2++) /* ラベルを最小ラベルでとりあえず設定 */
                            l_data[g][r2]=label_min;

                        for(i=0;i<num_con_label2;i++)
                            if(con_label2[i] != label_min) {
                                label_comb[0][num_label_comb]=label_min;
                                label_comb[1][num_label_comb]=con_label2[i];
                                num_label_comb++;
                            }
                    }
                } /* if(cont_flag == NO) OKからNOへ変化した場合、定義された１つのランについて処理する */
            }
            r++;
        } /* while(r < dim_retu) */
    } /* for(g=1;g<dim_gyou;g++) */

    if(num_label < 1) {
        return(0) ;
    }

    for(i=0;i<MaxLabel;i++)
        for(j=0;j<MaxLabel;j++)
	        label_matrix[i][j]=0;

    for(i=0;i<num_label_comb;i++) {
        label_matrix[label_comb[0][i]][label_comb[1][i]]=1;
        label_matrix[label_comb[1][i]][label_comb[0][i]]=1;
    }

    for(i=0;i<num_label;i++) {
        num_aite[i]=0;
        for(j=0;j<num_label;j++)
            if(label_matrix[i][j] == 1) {
                aite[i][num_aite[i]]=j;
                num_aite[i]++;
            }
    }

    for(this_label=0;this_label<num_label;this_label++)
        if(num_aite[this_label] > 0)
            for(i=0;i<num_aite[this_label];i++) {
                (void)add_to_label_matrix(this_label,aite[this_label][i],label_matrix,num_aite,aite);
            }

    num_label_group=0;
    for(this_label=0;this_label<num_label;this_label++) {
        if(num_aite[this_label] == 0) {/* 単独のランの場合 */
            label_member[num_label_group][0]=this_label;
            num_label_member[num_label_group]=1;
            num_label_group++;
        }
        else if(num_aite[this_label] > 0) {
            label_member[num_label_group][0]=this_label;
            num_label_member[num_label_group]=1;
            for(i=0;i<num_label;i++)
                if(label_matrix[this_label][i] == 1) {
                    label_member[num_label_group][num_label_member[num_label_group]]=i;
                    num_label_member[num_label_group]++;
                }
            num_label_group++;
        }
    }

    for(i=0;i<num_label_group;i++) /* ラベルの変換表作成 */
        for(j=0;j<num_label_member[i];j++)
            new_label[label_member[i][j]]=i;

    for(g=0;g<dim_gyou;g++)     /* labelをつけかえる */
        for(r=0;r<dim_retu;r++)
            if(l_data[g][r] >= 0)
                l_data[g][r]=new_label[l_data[g][r]];

    return num_label_group;
}

/***********************************************************************
 *
 * add_to_label_matrix -- labeling()が使う関数
 *
 * Syntux       : void add_to_label_matrix(int this_label, int aite_label,
 *			unsigned short label_matrix[][MaxLabel], int num_aite[],
 *			unsigned short aite[][MaxLabel])
 *       Input  : int this_label        ;
 *                int aite_label        ;
 *                int unsigned short label_matrix[][MaxLabel] ;
 *                int num_aite[]         ;
 *                int unsigned short aite[][MaxLabel] ;
 *
 ***********************************************************************/
void add_to_label_matrix(int this_label, int aite_label, unsigned short label_matrix[][MaxLabel], int num_aite[], unsigned short aite[][MaxLabel]) {
    int i;
    int aite_label2;
    int n_aite;

    n_aite=num_aite[aite_label];
    num_aite[aite_label]=-1;

    if(n_aite >= 0)
        label_matrix[this_label][aite_label]=1;

    for(i=0;i<n_aite;i++) {
        aite_label2=aite[aite_label][i];
        if(aite_label2 != this_label)
            (void)add_to_label_matrix(this_label,aite_label2,label_matrix,num_aite,aite);
    }
}
