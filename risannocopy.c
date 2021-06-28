#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define URLLEN (4096)
#define TITLELEN (1024)

// 辺リスト用の構造
struct Edge_t {
  int from;
  int to;
  struct Edge_t *next;
};

typedef struct Edge_t Edge;

// 隣接リスト表現のグラフ構造体
struct Graph_t {
  int n;
  int m;
  int _m;             // 内部使用：登録された辺の数
  Edge *_edges;       // 内部使用：全辺
  Edge **edges;       // 各頂点の最初の辺へのポインタ
  char **info;
};

typedef struct Graph_t Graph;

// 頂点数 n のグラフ（辺なし）を作成
Graph *create_empty_graph(int n)
{
  Graph *g;
  int i, j;
  g = malloc(sizeof(Graph));
  g->n = n;
  g->m = 0;
  g->edges = malloc(sizeof(Edge*)*n);
  for(i = 0; i < n; i++) g->edges[i] = NULL;
  g->_edges = NULL;
  g->info = malloc(sizeof(char*)*n);
  for(i = 0; i < n; i++) g->info[i] = NULL;
  return g;
}
// 頂点 vid に情報を付加（タイトルのみ保持）
void add_vertex_info(Graph *g, int vid, char *url, char *title)
{
  int l = strlen(title);
  if(g->info[vid] != NULL) free(g->info[vid]);
  g->info[vid] = malloc(l+1);
  strcpy(g->info[vid], title);
}

// エッジの数をあらかじめ指定しておく（エッジ情報のための領域確保をここで行う）
void reserve_edges(Graph *g, int m)
{
  g->m = m;
  g->_m = 0;
  g->_edges = malloc(sizeof(Edge)*m);
}

// from_vid から to_vid への辺を追加する
void add_edge(Graph *g, int from_vid, int to_vid)
{
  Edge *e = &g->_edges[g->_m++];
  e->to = to_vid;
  e->from = from_vid;
  e->next = g->edges[from_vid];
  g->edges[from_vid] = e;
}

// 辺を逆転したグラフを作る
Graph *reverse_graph(Graph *g)
{
  int i;
  Edge *e;
  Graph *rg = create_empty_graph(g->n);
  for(i = 0; i < g->n; i++) {
    add_vertex_info(rg, i, "", g->info[i]); // 頂点情報のコピー
  }
  reserve_edges(rg, g->m);
  for(i = 0; i < g->n; i++) {
    e = g->edges[i];    // 頂点 i の辺リストの最初の辺を取得
    while(e!=NULL) {          // 辺リストの終了まで
      add_edge(rg, e->to, e->from);  // 逆向きに辺を追加
      e = e->next;            // 辺リスト中の次の辺へ      
    }
  }
  return rg;
}


// 初回の深さ優先探索。flag は既に頂点に訪れたか否かのフラグ。頂点 v から探索を再帰的に進める
// stack は探索が終了した頂点を入れるスタック。sp はスタックポインタ。
void _dfs1(Graph *g, int *flag, int v, int *stack, int *sp)
{
  int j;
  Edge *e;
  flag[v] = 1;
  // 頂点 v の全ての辺に対して、その辺の先に訪れていないなら再帰呼び出し。
  e = g->edges[v];    // 頂点 v の辺リストの最初の辺を取得
  while(e!=NULL) {          // 辺リストの終了まで
    j = e->to;              // 辺の行き先の頂点
    // ここで、j への辺があり且つまだ j に訪れていないなら j へ再帰する。
    if (flag[j] == 0){
        _dfs1(g,flag,j,stack,sp);
    }
    e = e->next;            // 辺リスト中の次の辺へ
  }
  stack[(*sp)++] = v;  // 探索終了を記録
}

// 深さ優先探索。flag は既に頂点に訪れたか否かのフラグ。頂点 v から探索を再帰的に進める
// component は各頂点の属する強連結成分の番号を格納。c は現在探索中の強連結成分の番号
void _dfs2(Graph *g, int *flag, int v, int *component, int c)
{
  int j;
  Edge *e;
  flag[v] = 1;                 // この頂点に来たことを記録
  component[v] = c;            // この頂点の強連結成分の番号は c
  // 頂点 v の全ての辺に対して、その辺の先に訪れていないなら再帰呼び出し。
  e = g->edges[v];    // 頂点 v の辺リストの最初の辺を取得
  while(e!=NULL) {          // 辺リストの終了まで
    j = e->to;              // 辺の行き先の頂点
    // ここで、j への辺があり且つまだ j に訪れていないなら j へ再帰する。
    if (flag[j] == 0){
        _dfs2(g,flag,j,component,c);
    }
    e = e->next;            // 辺リスト中の次の辺へ
  }
}

// 強連結成分分解。component に各頂点の属する強連結成分の番号が入る
int scc(Graph *g, int *component)
{
  int i;
  int *flag = malloc(sizeof(int)*g->n);  // 探索済みかどうかのフラグ
  int *stack = malloc(sizeof(int)*g->n); // 初回深さ優先探索の終了順に頂点を格納する場所
  int sp = 0, c = 0;
  Graph *rg;
  
  for(i = 0; i < g->n; i++) flag[i] = 0; // フラグクリア
  for(i = 0; i < g->n; i++) {            // 一般には、複数の頂点から深さ優先探索をする
    if(flag[i] == 0) _dfs1(g, flag, i, stack, &sp); // 初回深さ優先探索
  }
  
  rg = reverse_graph(g);                 // 辺の向きを逆にしたグラフを作る
  
  for(i = 0; i < g->n; i++) flag[i] = 0; // フラグクリア
  for(sp = g->n-1; sp >=0; sp--) {       // 初回探索の終了の遅い順に
    if(flag[stack[sp]] == 0) {           // まだ訪れていない頂点から逆向きの深さ優先探索
      _dfs2(rg, flag, stack[sp], component, c);
      c++;                               // 探索が終了したら強連結成分がひとつ見つかる
    }
  }
  free(stack);
  free(flag);
  // 本来ならばここで rg を開放するが、面倒なので略。
  return c;                              // 強連結成分の個数
}

// PageRank
void pagerank(Graph *g, double *rank)
{
  int * ls = malloc(sizeof(int) * g->n);  // 出ていくリンク数
  double * new_rs = malloc(sizeof(double) * g->n); // 新しいランク用の配列
  Edge *e;
  double d = 0.85;                   // damping factor
  int i, v;
  double r, err, er;

  Graph *rg = reverse_graph(g);                 // 辺の向きを逆にしたグラフを作る

  // リンク数のカウント
  for(v = 0; v < g->n; v++) {
    ls[v] = 0;
    e = g->edges[v];          // 頂点 v の辺リストの最初の辺を取得
    while(e!=NULL) {          // 辺リストの終了まで
      ls[v]++;
      e = e->next;            // 辺リスト中の次の辺へ
    }
  }
  for(i = 0; i < g->n; i++) {  // 初期 rank
    rank[i] = 1.0/g->n;
  }
  
  for(;;) {
    for(v = 0; v < g->n; v++) { // 全頂点に対して
      r = 0; // 新しいランクを計算する一時変数
      // 逆辺に対して (つまり、v に入ってくる辺に関して)
      e = rg->edges[v];          // 頂点 v の辺リストの最初の辺を取得
      while(e!=NULL) {          // 辺リストの終了まで
        // ランク加算
        r = r + rank[e->to]/ls[e->to];
        e = e->next;            // 辺リスト中の次の辺へ
      }
      new_rs[v] = (1-d)/g->n + d * r; // 新しいランク値
    }
    err = 0;            // ランクの更新量
    for(v = 0; v < g->n; v++) {
      er = rank[v] - new_rs[v];
      err += er*er;
      rank[v] = new_rs[v];
    }
    if(err < 1e-6) break;      // 不動点で繰り返し終了
  }
}

void mygets(char* str, int count);

int main(int argc, char *argv[])
{
  int n, m, i, vid, from_vid, to_vid, ret;
  char url[URLLEN];
  char title[TITLELEN];
  Graph *g;
  mygets(url, URLLEN); //gets(url);
  ret = scanf("%d", &n);

  g = create_empty_graph(n);


  for(i = 0; i < n; i++) {
    ret = scanf("%d", &vid);
    ret = scanf("%s", url);
    mygets(title, TITLELEN); //gets(title);
    add_vertex_info(g, vid, url, title);
  }
  ret = scanf("%d", &m);
  reserve_edges(g, m);
  for(i = 0; i < m; i++) {
    ret = scanf("%d", &from_vid);
    ret = scanf("%d", &to_vid);
    add_edge(g, from_vid, to_vid);
  }

  // ここまでで Graph の情報の読み込みが完了

  // 強連結成分分解
  {
    int *component = malloc(sizeof(int) * g->n);
    int j;
    scc(g, component);
    
    printf("digraph{\n");
    for(i = 0; i < g->n; i++) {
      printf("%d [label=\"%d\"]\n", i, component[i]);
    }
    for(i = 0; i < g->n; i++) {
      Edge *e = g->edges[i];    // 頂点 i の辺リストの先頭取得
      while(e!=NULL) {          // 辺リストの終了まで
        printf("%d -> %d\n", e->from, e->to);
        e = e->next;            // 辺リスト中の次の辺へ
      }
    }
    printf("}\n");
  }
  // PageRank
  {
    double *rank = malloc(sizeof(double) * g->n);
    pagerank(g, rank);
    
    for(i = 0; i < g->n; i++) {
      printf("// rank of %d is %g\n", i, rank[i]);
    }
  }
  return 0;
}

void mygets(char* str, int count) {
  int c;
  while(count-- > 0) {
    c = getchar();
    if(c == '\n' || c == EOF) {
      *str = '\0';
      return;
    }
    else if(c == '\r') {
      ++count;
      continue;
    }
    *(str++) = c;
  }
  *(str-1) = '\0';
  do{
    c = getchar();
  }while(c != '\n' && c != EOF);
  return;
}