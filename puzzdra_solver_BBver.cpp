/*
puzzdra_solver

パズドラのルート解析プログラムです

コンパイラはMinGWを推奨します

コマンドは以下の通りです
g++ -O2 -std=c++11 -fopenmp -mbmi2 puzzdra_solver_BBver.cpp -o puzzdra_solver_BBver

なるべく少ない時間でなるべく大きいコンボを出したいです

printf("TotalDuration:%fSec\n", t_sum);
printf("Avg.NormalCombo #:%f/%f\n", avg / (double)i, MAXCOMBO / (double)i);

これらが改善されればpull request受け付けます

チェック1：これを10コンボできるか

962679
381515
489942
763852
917439

914769
264812
379934
355886
951279

チェック2：1000盤面平均落ちコンボ数が9.20付近か

チェック3：1000盤面平均コンボ数が理論値付近か

全チェック達成したら合格


*/
#pragma warning(disable:4710)
#pragma warning(disable:4711)
#pragma warning(disable:4820)
#include <vector>
#include <cfloat>
#include <cstdio>
#include <cstring>
#include <climits>
#include <ctime>
#include <cstdlib>
#include <cmath>
#include <string>
#include <iostream>
#include <cstdint>
#include <algorithm>
#include <cassert>
#include <random>
#include <queue>
#include <deque>
#include <list>
#include <map>
#include <array>
#include <chrono>
#include <fstream>
#include <functional>
#include <unordered_map>
#include <immintrin.h>
#ifdef _OPENMP
#include <omp.h>
#endif
using namespace std;
#define DLT(ST,ED) ((double)((ED)-(ST))/CLOCKS_PER_SEC)//時間差分
#define XX(PT)? ((PT)&15)
#define YY(PT)? XX((PT)>>4)
#define YX(Y,X) ((Y)<<4|(X))
#define DIR 4//方向
#define ROW 5//変更不可
#define COL 6//変更不可
#define DROP 6//ドロップの種類//MAX9
#define TRN? 155//手数//MAX155
#define STP YX(7,7)//無効手[無効座標]
#define MAX_TURN 150//最大ルート長//MAX150
#define BEAM_WIDTH 10000//ビーム幅//MAX200000
#define PROBLEM 1000//問題数
#define BONUS 10//評価値改善係数
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define NODE_SIZE MAX(500,4*BEAM_WIDTH)
typedef char F_T;//盤面型
typedef char T_T;//手数型
typedef unsigned long long ll;
enum { EVAL_NONE = 0, EVAL_FALL, EVAL_SET, EVAL_FS, EVAL_COMBO };
void init(F_T field[ROW][COL]); //初期配置生成関数
void fall(int x,int h,F_T field[ROW][COL]); //ドロップの落下処理関数
void set(F_T field[ROW][COL], int force); //空マスを埋める関数
void show_field(F_T field[ROW][COL]); //盤面表示関数
unsigned int rnd(int mini, int maxi); //整数乱数
//上下左右に連結しているドロップを再帰的に探索していく関数
int chain(int nrw, int ncl, F_T d, F_T field[ROW][COL], F_T chkflag[ROW][COL], F_T delflag[ROW][COL]);
int evaluate(F_T field[ROW][COL], int flag); //コンボ数判定関数
int sum_e(F_T field[ROW][COL]);//落とし有り、落ちコン無しコンボ数判定関数
int sum_evaluate(F_T field[ROW][COL]);//落としも落ちコンも有りコンボ数判定関数
void operation(F_T field[ROW][COL], T_T route[TRN],ll dropBB[DROP+1]); //スワイプ処理関数

int evaluate2(F_T field[ROW][COL], int flag, int* combo, ll* hash);//落とし減点評価関数
int sum_e2(F_T field[ROW][COL], int* combo, ll* hash);//評価関数

ll xor128();//xorshift整数乱数
ll zoblish_field[ROW][COL][DROP+1];

ll sqBB[64];
int evaluate3(ll dropBB[DROP+1], int flag, int* combo, ll* hash);//落とし減点評価関数
int sum_e3(ll dropBB[DROP+1], int* combo, ll* hash);//評価関数
ll around(ll bitboard);
int table[64];
ll fill_64[64];
ll file_bb[COL];
ll calc_mask(ll bitboard);
ll fallBB(ll p,ll rest,ll mask);

struct node {//どういう手かの構造体
	T_T movei[TRN];//スワイプ移動座標
	int score;//評価値
	int combo;//コンボ数
	int nowC;//今どのx座標にいるか
	int nowR;//今どのy座標にいるか
	int prev;//1手前は上下左右のどっちを選んだか
	int prev_score;//1手前の評価値
	int improving;//評価値改善回数
	ll hash;//盤面のハッシュ値
	node() {//初期化
		this->score = 0;
		this->prev = -1;
		//memset(this->movei, STP, sizeof(this->movei));
	}
	bool operator < (const node& n)const {//スコアが高い方が優先される
		return score < n.score;
	}
}fff[NODE_SIZE];
struct Action {//最終的に探索された手
	int score;//コンボ数
	int maxcombo;//理論コンボ数
	T_T moving[TRN];//スワイプ移動座標
	Action() {//初期化
		this->score = 0;
		//memset(this->moving, STP, sizeof(this->moving));
	}
};
Action BEAM_SEARCH(F_T f_field[ROW][COL]); //ルート探索関数
double part1 = 0, part2 = 0, part3 = 0, part4 = 0, MAXCOMBO = 0;
Action BEAM_SEARCH(F_T f_field[ROW][COL]) {

	int stop = 0;//理論最大コンボ数

	int drop[DROP + 1] = { 0 };
	for (int row = 0; row < ROW; row++) {
		for (int col = 0; col < COL; col++) {
			if (1 <= f_field[row][col] && f_field[row][col] <= DROP) {
				drop[f_field[row][col]]++;
			}
		}
	}
	for (int i = 1; i <= DROP; i++) {
		stop += drop[i] / 3;
	}
	MAXCOMBO += (double)stop;

	deque<node>dque;
	double start, st;
	//1手目を全通り探索する
	dque.clear();
	for (int i = 0; i < ROW; i++) {
		for (int j = 0; j < COL; j++) {
			node cand;
			cand.nowR = i;//y座標
			cand.nowC = j;//x座標
			cand.prev = -1;//1手前はない
			cand.movei[0] = (T_T)YX(i, j);//1手目のyx座標
			for (int trn = 1; trn < TRN; trn++) {
				cand.movei[trn] = STP;
			}
			F_T ff_field[ROW][COL];
			memcpy(ff_field,f_field,sizeof(ff_field));
			int cmb;
			ll ha;
			cand.prev_score=sum_e2(ff_field,&cmb,&ha);
			cand.improving=0;
			cand.hash=ha;
			dque.push_back(cand);
		}
	}? ? ? ?  // L, U,D,R //
	int dx[DIR] = { -1, 0,0,1 },
		dy[DIR] = { 0,-1,1,0 };
	Action bestAction;//最善手
	int maxValue = 0;//最高スコア

	bestAction.maxcombo = stop;
	unordered_map<ll, bool> checkNodeList[ROW*COL];

	ll rootBB[DROP+1]={0};

	for(int row=0;row<ROW;row++){
	for(int col=0;col<COL;col++){
	int pos=63-((8*col)+row+10);
	rootBB[f_field[row][col]]|=(1ll << (pos));
	}
	}

	//2手目以降をビームサーチで探索
	for (int i = 1; i < MAX_TURN; i++) {
		int ks = (int)dque.size();
		start = omp_get_wtime();
#pragma omp parallel for private(st),reduction(+:part1,part4)
		for (int k = 0; k < ks; k++) {
#ifdef _OPENMP
			if (i == 1 && k == 0) {
				printf("Threads[%d/%d]\n",
					omp_get_num_threads(),
					omp_get_max_threads());
			}
#endif
			node temp = dque[k];//que.front(); que.pop();
			F_T temp_field[ROW][COL];
			ll temp_dropBB[DROP+1]={0};
			memcpy(temp_field, f_field, sizeof(temp_field));
			memcpy(temp_dropBB,rootBB,sizeof(rootBB));
			operation(temp_field, temp.movei,temp_dropBB);
			for (int j = 0; j < DIR; j++) {//上下左右の4方向が発生
				node cand = temp;
				if (0 <= cand.nowC + dx[j] && cand.nowC + dx[j] < COL &&
					0 <= cand.nowR + dy[j] && cand.nowR + dy[j] < ROW) {
					if (cand.prev + j != 3) {
						int ny=cand.nowR + dy[j];
						int nx=cand.nowC + dx[j];
						F_T field[ROW][COL];//盤面
						ll dropBB[DROP+1]={0};
						memcpy(field,temp_field,sizeof(temp_field));//盤面をもどす
						memcpy(dropBB,temp_dropBB,sizeof(temp_dropBB));
						F_T tmp=field[cand.nowR][cand.nowC];
						int pre_drop=(int)tmp;
						int pre_pos=63-((8*cand.nowC)+cand.nowR+10);
						int next_drop=(int)field[ny][nx];
						int next_pos=63-((8*nx)+ny+10);
						dropBB[pre_drop]^=(sqBB[pre_pos]|sqBB[next_pos]);
						dropBB[next_drop]^=(sqBB[pre_pos]|sqBB[next_pos]);
						field[cand.nowR][cand.nowC]=field[ny][nx];
						field[ny][nx]=tmp;
						cand.nowC += dx[j];
						cand.nowR += dy[j];
						cand.movei[i] = (T_T)YX(cand.nowR, cand.nowC);
						st = omp_get_wtime();
						int cmb;
						ll ha;
						cand.score = sum_e3(dropBB, &cmb,&ha);
						cand.combo = cmb;
						cand.hash=ha;
						part1 += omp_get_wtime() - st;
						cand.prev = j;
						st = omp_get_wtime();
						//#pragma omp critical
											//{ pque.push(cand); }
						fff[(4 * k) + j] = cand;
						part4 += omp_get_wtime() - st;
					}
					else {
						cand.combo = -1;
						fff[(4 * k) + j] = cand;
					}
				}
				else {
					cand.combo = -1;
					fff[(4 * k) + j] = cand;
				}
			}
		}
		part2 += omp_get_wtime() - start;
		start = omp_get_wtime();
		dque.clear();
		vector<pair<int, int> >vec;
		int ks2 = 0;
		for (int j = 0; j < 4 * ks; j++) {
			if (fff[j].combo != -1) {
				if(fff[j].score>fff[j].prev_score){fff[j].improving=fff[j].improving+1;}
				fff[j].prev_score=fff[j].score;
				vec.push_back(make_pair(fff[j].score+(BONUS*fff[j].improving), j));
				ks2++;
			}
		}
		sort(vec.begin(), vec.end());
		int push_node=0;
		for (int j = 0; push_node < BEAM_WIDTH && j < ks2; j++) {
			node temp = fff[vec[ks2-1-j].second];
			if (maxValue < temp.combo) {//コンボ数が増えたらその手を記憶する
				maxValue = temp.combo;
				bestAction.score = maxValue;
				memcpy(bestAction.moving, temp.movei, sizeof(temp.movei));
				//コンボ数が理論値になったらreturn
				if (temp.combo == stop) { return bestAction; }
			}
			if (i < MAX_TURN - 1) {
			int pos=(temp.nowR*COL)+temp.nowC;
			if(!checkNodeList[pos][temp.hash]){
				checkNodeList[pos][temp.hash]=true;
				dque.push_back(temp);
				push_node++;
			}
			}
		}
		part3 += omp_get_wtime() - start;
	}
	return bestAction;
}
void show_field(F_T field[ROW][COL]) {
	for (int i = 0; i < ROW; i++) {
		for (int j = 0; j < COL; j++) {
			printf("%d", field[i][j]);
		}
		printf("\n");
	}
}
void fall(int x,int h,F_T field[ROW][COL]) {
		int tgt;
		for (tgt = ROW - 1; tgt >= h && field[tgt][x] != 0; tgt--);
		for (int i = tgt - 1; i >= h; i--) {
			if (field[i][x] != 0) {
				F_T c = field[i][x];
				field[i][x] = 0;
				field[tgt][x] = c;
				tgt--;
			}
		}
}
void init(F_T field[ROW][COL]) { set(field, !0); }
void set(F_T field[ROW][COL], int force) {
	for (int i = 0; i < ROW; i++) {
		for (int j = 0; j < COL; j++) {
			if (field[i][j] == 0 || force) {//空マスだったらうめる
				field[i][j] = (F_T)rnd(force ? 0 : 1, DROP);//1-DROPの整数乱数
			}
		}
	}
}
int chain(int nrw, int ncl, F_T d, F_T field[ROW][COL],
	F_T chkflag[ROW][COL], F_T delflag[ROW][COL]) {
	int count = 0;
#define CHK_CF(Y,X) (field[Y][X] == d && chkflag[Y][X]==0 && delflag[Y][X] > 0)
	//連結している同じ色の消去ドロップが未探索だったら
	if (CHK_CF(nrw, ncl)) {
		++count; //連結ドロップ数の更新
		chkflag[nrw][ncl]=1;//探索済みにする
			//以下上下左右に連結しているドロップを再帰的に探索していく
		if (0 < nrw && CHK_CF(nrw - 1, ncl)) {
			count += chain(nrw - 1, ncl, d, field, chkflag, delflag);
		}
		if (nrw < ROW - 1 && CHK_CF(nrw + 1, ncl)) {
			count += chain(nrw + 1, ncl, d, field, chkflag, delflag);
		}
		if (0 < ncl && CHK_CF(nrw, ncl - 1)) {
			count += chain(nrw, ncl - 1, d, field, chkflag, delflag);
		}
		if (ncl < COL - 1 && CHK_CF(nrw, ncl + 1)) {
			count += chain(nrw, ncl + 1, d, field, chkflag, delflag);
		}
	}
	return count;
}
int evaluate(F_T field[ROW][COL], int flag) {
	int combo = 0;

	while (1) {
		int cmb = 0;
		F_T chkflag[ROW][COL]={0};
		F_T delflag[ROW][COL]={0};
		F_T GetHeight[COL];
		for (int row = 0; row < ROW; row++) {
			for (int col = 0; col < COL; col++) {
				F_T num=field[row][col];
				if(row==0){
				GetHeight[col]=(F_T)ROW;
				}
				if(num>0 && GetHeight[col]==(F_T)ROW){
				GetHeight[col]=(F_T)row;
				}
				if (col <= COL - 3 && num == field[row][col + 1] && num == field[row][col + 2] && num > 0) {
					delflag[row][col]=1;
					delflag[row][col+1]=1;
					delflag[row][col+2]=1;
				}
				if (row <= ROW - 3 && num == field[row + 1][col] && num == field[row + 2][col] && num > 0) {
					delflag[row][col]=1;
					delflag[row+1][col]=1;
					delflag[row+2][col]=1;
				}
			}
		}
		for (int row = 0; row < ROW; row++) {
			for (int col = 0; col < COL; col++) {
				if (delflag[row][col] > 0) {
					if (chain(row, col, field[row][col], field, chkflag, delflag) >= 3) {
						cmb++;
					}
				}
			}
		}
		combo += cmb;
		//コンボが発生しなかったら終了
		if (cmb == 0 || 0 == (flag & EVAL_COMBO)) { break; }
		for (int row = 0; row < ROW; row++) {
			for (int col = 0; col < COL; col++) {
				//コンボになったドロップは空になる
				if (delflag[row][col]> 0) { field[row][col] = 0; }
			}
		}

		if (flag & EVAL_FALL){
		for(int x=0;x<COL;x++){
		fall(x,GetHeight[x],field);
		}
		}//落下処理発生
		if (flag & EVAL_SET){set(field, 0);}//落ちコン発生

	}
	return combo;
}
int evaluate2(F_T field[ROW][COL], int flag, int* combo, ll* hash) {
	int ev = 0;
	*combo = 0;
	ll ha=0;
	int oti = 0;
	while (1) {
		int cmb = 0;
		int cmb2 = 0;
		F_T chkflag[ROW][COL]={0};
		F_T delflag[ROW][COL]={0};
		F_T GetHeight[COL];
		for (int row = 0; row < ROW; row++) {
			for (int col = 0; col < COL; col++) {
				F_T num = field[row][col];
				if(row==0){
				GetHeight[col]=(F_T)ROW;
				}
				if(num>0 && GetHeight[col]==(F_T)ROW){
				GetHeight[col]=(F_T)row;
				}
				if(oti==0){
					ha ^= zoblish_field[row][col][(int)num];
				}
				if (col <= COL - 3 && num == field[row][col + 1] && num == field[row][col + 2] && num > 0) {
					delflag[row][col]=1;
					delflag[row][col+1]=1;
					delflag[row][col+2]=1;
				}
				if (row <= ROW - 3 && num == field[row + 1][col] && num == field[row + 2][col] && num > 0) {
					delflag[row][col]=1;
					delflag[row+1][col]=1;
					delflag[row+2][col]=1;
				}
			}
		}

		F_T cnt[DROP + 1] = { 0 };
		F_T drop[DROP + 1][ROW * COL][2] = { 0 };

		for (int row = 0; row < ROW; row++) {
			for (int col = 0; col < COL; col++) {
				drop[field[row][col]][cnt[field[row][col]]][0] = (F_T)row;
				drop[field[row][col]][cnt[field[row][col]]][1] = (F_T)col;
				cnt[field[row][col]]++;
				if (delflag[row][col]>0) {
					int c = chain(row, col, field[row][col], field, chkflag, delflag);
					if (c >= 3) {
						cmb++;
						if (c == 3) { cmb2 += 30; }
						else { cmb2 += 20; }
					}
				}
			}
		}
		F_T erase_x[COL]={0};
		for (int i = 1; i <= DROP; i++) {
			for (int j = 0; j < cnt[i] - 1; j++) {
				int d1 = (int)drop[i][j][0];
				int d2 = (int)drop[i][j][1];
				int d3 = (int)drop[i][j + 1][0];
				int d4 = (int)drop[i][j + 1][1];
				int add = max(d1 - d3, d3 - d1) + max(d2 - d4, d4 - d2);
				add += add;
				add /= 3;
				cmb2 -= add;
				if (delflag[d1][d2]> 0) {
					field[d1][d2] = 0;
					erase_x[d2]=1;
				}
				if (delflag[d3][d4] > 0) {
					field[d3][d4] = 0;
					erase_x[d4]=1;
				}
			}
		}
		*combo += cmb;
		ev += cmb2;
		//コンボが発生しなかったら終了
		if (cmb == 0 || 0 == (flag & EVAL_COMBO)) { break; }
		oti++;
		if (flag & EVAL_FALL){//落下処理発生
		for(int x=0;x<COL;x++){
		if(erase_x[x]==1){
		fall(x,GetHeight[x],field);
		}
		}
		}
		if (flag & EVAL_SET){set(field, 0);}//落ちコン発生

	}
	ev += oti;
	*hash=ha;
	return ev;
}
int evaluate3(ll dropBB[DROP+1], int flag, int* combo, ll* hash) {
	int ev = 0;
	*combo = 0;
	ll ha=0;
	int oti = 0;
	ll ha2;
	ll occBB=0;

	for(int i=1;i<=DROP;i++){
	occBB|=dropBB[i];
	}

	while (1) {
		int cmb = 0;
		int cmb2 = 0;

		ll linked[DROP+1]={0};

		for(int i=1;i<=DROP;i++){
		ll vert = (dropBB[i]) & (dropBB[i] << 1) & (dropBB[i] << 2);
		ll hori = (dropBB[i]) & (dropBB[i] << 8) & (dropBB[i] << 16);

		linked[i]=vert | (vert >> 1) | (vert >> 2) | hori | (hori >> 8) | (hori >> 16);
		}

		for(int i=1;i<=DROP;i++){
		long long tmp_linked=(long long)linked[i];
		while(1){
		long long chainBB=tmp_linked&(-tmp_linked);
		if(chainBB==0ll){break;}
		long long tmp=chainBB;
		while (1) {
		if(tmp==(tmp_linked&(long long)around(tmp))){break;}
		tmp=tmp_linked&(long long)around(tmp);
		}
		int c=__builtin_popcountll(tmp);
		tmp_linked^=tmp;
		if(c>=3){
		cmb++;
		if(c==3){cmb2+=30;}
		else{cmb2+=20;}
		}
		}
		}


		for(int i=1;i<=DROP;i++){

		long long tmp_drop=(long long)dropBB[i];

		int prev_x;
		int prev_y;

		for(int j=0;;j++){
		long long t=tmp_drop&(-tmp_drop);
		ll exist=(ll)t;
		if(exist==0ll){break;}
		int h=( int ) ( ( exist * 0x03F566ED27179461ULL ) >> 58 );
		int pos=table[h];
		int pos_x=(53-pos)/8;
		int pos_y=(53-pos)%8;
		if(oti==0){
		ha ^= zoblish_field[pos_y][pos_x][i];
		}
		if(j>0){
		int add=max(pos_x-prev_x,prev_x-pos_x)+max(pos_y-prev_y,prev_y-pos_y);
		add+=add;
		add/=3;
		cmb2-=add;
		}
		prev_x=pos_x;
		prev_y=pos_y;
		tmp_drop=tmp_drop & ~(1ll<<(pos));
		}
		}
		for(int i=1;i<=DROP;i++){
		dropBB[i]^=linked[i];
		occBB^=linked[i];
		}

		*combo += cmb;
		ev += cmb2;
		//コンボが発生しなかったら終了
		if (cmb == 0 || 0 == (flag & EVAL_COMBO)) { break; }
		oti++;

		ll mask=calc_mask(occBB);

		for(int i=1;i<=DROP;i++){
		dropBB[i]=fallBB(dropBB[i],occBB,mask);
		}
		occBB=fallBB(occBB,occBB,mask);

		/*

		F_T chkflag[ROW][COL]={0};
		F_T delflag[ROW][COL]={0};

		F_T GetHeight[COL];
		for (int row = 0; row < ROW; row++) {
			for (int col = 0; col < COL; col++) {
				F_T num = field[row][col];
				if(row==0){
				GetHeight[col]=(F_T)ROW;
				}
				if(num>0 && GetHeight[col]==(F_T)ROW){
				GetHeight[col]=(F_T)row;
				}
				if(oti==0){
					ha2 ^= zoblish_field[row][col][(int)num];
				}
				if (col <= COL - 3 && num == field[row][col + 1] && num == field[row][col + 2] && num > 0) {
					delflag[row][col]=1;
					delflag[row][col+1]=1;
					delflag[row][col+2]=1;
				}
				if (row <= ROW - 3 && num == field[row + 1][col] && num == field[row + 2][col] && num > 0) {
					delflag[row][col]=1;
					delflag[row+1][col]=1;
					delflag[row+2][col]=1;
				}
			}
		}


		F_T cnt[DROP + 1] = { 0 };
		F_T drop[DROP + 1][ROW * COL][2] = { 0 };
		for (int row = 0; row < ROW; row++) {
			for (int col = 0; col < COL; col++) {
				drop[field[row][col]][cnt[field[row][col]]][0] = (F_T)row;
				drop[field[row][col]][cnt[field[row][col]]][1] = (F_T)col;
				cnt[field[row][col]]++;
				if (delflag[row][col]>0) {
					int c = chain(row, col, field[row][col], field, chkflag, delflag);
					if (c >= 3) {
						cmb++;
						if (c == 3) { cmb2 += 30; }
						else { cmb2 += 20; }
					}
				}
			}
		}

		F_T erase_x[COL]={0};
		for (int i = 1; i <= DROP; i++) {
			for (int j = 0; j < cnt[i] - 1; j++) {
				int d1 = (int)drop[i][j][0];
				int d2 = (int)drop[i][j][1];
				int d3 = (int)drop[i][j + 1][0];
				int d4 = (int)drop[i][j + 1][1];
				int add = max(d1 - d3, d3 - d1) + max(d2 - d4, d4 - d2);
				add += add;
				add /= 3;
				cmb2 -= add;
				if (delflag[d1][d2]> 0) {
					field[d1][d2] = 0;
					erase_x[d2]=1;
				}
				if (delflag[d3][d4] > 0) {
					field[d3][d4] = 0;
					erase_x[d4]=1;
				}
			}
		}
		*combo += cmb;
		ev += cmb2;
		//コンボが発生しなかったら終了
		if (cmb == 0 || 0 == (flag & EVAL_COMBO)) { break; }
		oti++;
		if (flag & EVAL_FALL){//落下処理発生
		for(int x=0;x<COL;x++){
		if(erase_x[x]==1){
		fall(x,GetHeight[x],field);
		}
		}
		}
		if (flag & EVAL_SET){set(field, 0);}//落ちコン発生
*/

	}
	ev += oti;
	*hash=ha;
	return ev;
}
int sum_e3(ll dropBB[DROP+1], int* combo, ll* hash) {//落とし有り、落ちコン無し評価関数
	return evaluate3(dropBB, EVAL_FALL | EVAL_COMBO, combo,hash);
}
int sum_e2(F_T field[ROW][COL], int* combo, ll* hash) {//落とし有り、落ちコン無し評価関数
	return evaluate2(field, EVAL_FALL | EVAL_COMBO, combo,hash);
}
int sum_e(F_T field[ROW][COL]) {//落とし有り、落ちコン無しコンボ数判定関数
	return evaluate(field, EVAL_FALL | EVAL_COMBO);
}
int sum_evaluate(F_T field[ROW][COL]) {//落としも落ちコンも有りコンボ数判定関数
	return evaluate(field, EVAL_FS | EVAL_COMBO);
}
//移動した後の盤面を生成する関数
void operation(F_T field[ROW][COL], T_T route[TRN],ll dropBB[DROP+1]) {
	int prw = (int)YY(route[0]), pcl = (int)XX(route[0]), i;
	for (i = 1; i < MAX_TURN; i++) {
		if (route[i] == STP) { break; }
		//移動したら、移動前ドロップと移動後ドロップを交換する
		int row = (int)YY(route[i]), col = (int)XX(route[i]);
		int pre_drop=(int)field[prw][pcl];
		int pre_pos=63-((8*pcl)+prw+10);
		int next_drop=(int)field[row][col];
		int next_pos=63-((8*col)+row+10);
		dropBB[pre_drop]^=(sqBB[pre_pos]|sqBB[next_pos]);
		dropBB[next_drop]^=(sqBB[pre_pos]|sqBB[next_pos]);
		F_T c = field[prw][pcl];
		field[prw][pcl] = field[row][col];
		field[row][col] = c;
		prw = row, pcl = col;
	}
}
ll around(ll bitboard){
return bitboard | bitboard >> 1 | bitboard << 1 | bitboard >> 8 | bitboard << 8;
}
unsigned int rnd(int mini, int maxi) {
	static mt19937 mt((int)time(0));
	uniform_int_distribution<int> dice(mini, maxi);
	return dice(mt);
}
ll xor128() {//xorshift整数乱数
	static unsigned long long rx = 123456789, ry = 362436069, rz = 521288629, rw = 88675123;
	ll rt = (rx ^ (rx << 11));
	rx = ry; ry = rz; rz = rw;
	return (rw = (rw ^ (rw >> 19)) ^ (rt ^ (rt >> 8)));
}
ll calc_mask(ll bitboard){

ll result = (fill_64[__builtin_popcountll((bitboard & file_bb[0]))] << 48)|(fill_64[__builtin_popcountll((bitboard & file_bb[1]))] << 40) |
(fill_64[__builtin_popcountll((bitboard & file_bb[2]))] << 32) | (fill_64[__builtin_popcountll((bitboard & file_bb[3]))] << 24) |
(fill_64[__builtin_popcountll((bitboard & file_bb[4]))] << 16) | (fill_64[__builtin_popcountll((bitboard & file_bb[5]))] << 8);

return result;

}
ll fallBB(ll p,ll rest, ll mask)
{
p = _pext_u64(p, rest);
p = _pdep_u64(p, mask);
return p;
}

int main() {

	int i, j, k;
	for(i=0;i<ROW;++i){
	for(j=0;j<COL;++j){
	for(k=0;k<=DROP;k++){
	zoblish_field[i][j][k]=xor128();
	}
	}
	}

	for(i=0;i<ROW;i++){
	for(j=0;j<COL;j++){
	int pos=63-((8*j)+i+10);
	sqBB[pos]|=1ll<<pos;
	}
	}
	ll ha = 0x03F566ED27179461ULL;
	for (i = 0; i < 64; i++){
	table[ ha >> 58 ] = i;
	ha <<= 1;
	}

	ll res=2ll;
	fill_64[1]=res;
	for(i=2;i<64;i++){
	fill_64[i]=res+(1ll<<i);
	res=fill_64[i];
	}

	int po=53;
	for(i=0;i<COL;i++){
	for(j=0;j<ROW;j++){
	file_bb[i] |= (1ll << (po-j));
	}
	po-=8;
	}

	double avg = 0;//平均コンボ数
	double start;
	double t_sum = 0;
	double oti_avg = 0;//平均落ちコンボ数
	for (i = 0; i < PROBLEM; i++) {//PROBLEM問解く
		F_T f_field[ROW][COL]; //スワイプ前の盤面
		F_T field[ROW][COL]; //盤面
		F_T oti_field[ROW][COL];//落ちコン用盤面
		printf("input:No.%d/%d\n", i + 1, PROBLEM);
		init(f_field); set(f_field, 0);//初期盤面生成
		show_field(f_field);//盤面表示
		/*
		for (j = 0; j < ROW; j++) {
			string s = "";
			cin >> s;
			for (k = 0; k < COL; k++) {
				f_field[j][k] = s[k] - '0';
			}
		}
		*/
		start = omp_get_wtime();
		Action tmp = BEAM_SEARCH(f_field);//ビームサーチしてtmpに最善手を保存
		double diff = omp_get_wtime() - start;
		t_sum += diff;
		printf("(x,y)=(%d,%d)", XX(tmp.moving[0]), YY(tmp.moving[0]));
		for (j = 1; j < MAX_TURN; j++) {//y座標は下にいくほど大きくなる
			if (tmp.moving[j] == STP) { break; }
			if (XX(tmp.moving[j]) == XX(tmp.moving[j - 1]) + 1) { printf("R"); } //"RIGHT"); }
			if (XX(tmp.moving[j]) == XX(tmp.moving[j - 1]) - 1) { printf("L"); } //"LEFT"); }
			if (YY(tmp.moving[j]) == YY(tmp.moving[j - 1]) + 1) { printf("D"); } //"DOWN"); }
			if (YY(tmp.moving[j]) == YY(tmp.moving[j - 1]) - 1) { printf("U"); } //"UP"); }
		} printf("\n");
		memcpy(field, f_field, sizeof(f_field));
		ll BB[DROP+1];
		operation(field, tmp.moving,BB);
		printf("output:No.%d/%d\n", i + 1, PROBLEM);
		show_field(field);
		memcpy(oti_field, field, sizeof(field));
		int combo = sum_e(field);
		int oti = sum_evaluate(oti_field);
		printf("Normal:%d/%dCombo\n", combo, tmp.maxcombo);
		printf("Oti:%dCombo\n", oti);
		printf("elapsed time:%fSec\n", diff);
		printf("------------\n");
		avg += (double)combo;
		oti_avg += (double)oti;
	}
	printf("TotalDuration:%fSec\n", t_sum);
	printf("Avg.NormalCombo #:%f/%f\n", avg / (double)i, MAXCOMBO / (double)i);
	printf("Avg.OtiCombo #:%f\n", oti_avg / (double)i);
	printf("p1:%f,p2:%f,p3:%f,p4:%f\n", part1, part2, part3, part4);
	j = getchar();
	return 0;
}
