/* Copyright (C) 2021, Murad Banaji
 *
 * This is TownVillage.cc, a toy model, part of COVIDAGENT.
 *
 * COVIDAGENT is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License 
 * as published by the Free Software Foundation; either version 3, 
 * or (at your option) any later version.
 *
 * COVIDAGENT is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with COVIDAGENT: see the file COPYING.  If not, see 
 * <https://www.gnu.org/licenses/>

 */

 /* 
 * Compilation instructions in README
 * A lot of unnecessary complexity here is historical - 
 * associated with the way code was adapted from previous projects.
 * Perhaps future versions will simplify the code.
 */

#include <limits>
#include "TownVillage.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h> // random seeding
#include <ctype.h>
#include <string.h>
#include <iostream>
#include <random>

// Maximum number of infected individuals - memory limit?
#define MAXINFS 2000000

#define maxx(A, B) ((A) > (B) ? (A) : (B))

//Externally declared (bad practice I know!)

int inflist[MAXINFS]; //For book-keeping free spaces in list
int sero_max[MAXINFS];
int sero_final[MAXINFS];
int sero_cur[MAXINFS];
int sero_time[MAXINFS];
int inf_ages[MAXINFS];

std::default_random_engine generator;

int getline(FILE *fp, char s[], int lim)
{
  /* store a line as a string, including the terminal newline character */
  int c=0, i;

  for(i=0; i<lim-1 && (c=getc(fp))!=EOF && c!='\n';++i)
    s[i] = c;
  if (c == '\n') {
    s[i] = c;
    ++i;
  }
  s[i] = '\0';
  return i;
}

//From https://stackoverflow.com/questions/29787310/does-pow-work-for-int-data-type-in-c
int int_pow(int base, int exp){
  int result = 1;
  while (exp)
    {
      if (exp % 2)
	result *= base;
      exp /= 2;
      base *= base;
    }
  return result;
}

int getnthblock(char *s, char *v, int len, int n){
  // get the nth valid block (only spaces count as separators) from a string s and put it in v. Returns the next position in  s. 
  int i, j, k;
  i=0, k=0;
  if(len < 2){
    fprintf(stderr, "ERROR in getnthblock in inf2.cc: third argument must be at least 2.\n");
    return 0;
  }
  v[0] = '\0'; // in case we have an empty string, return an empty word.
  while(s[k] != '\0'){
    j=0;
    while(isspace((int) s[k])) // skip space
      k++;
    for(i=0;i<n-1;i++){
      while(!(isspace((int) s[k]))) // skip first word 
        k++;
      while(isspace((int) s[k])) //skip space
        k++;
    }
    while((j<len-1) && !(isspace((int) s[k]))){ // get the word
      v[j++] = s[k++];
    }
    v[j++] = '\0';
    if(j==len){
      fprintf(stderr, "WARNING: in routine getnthblock in file inf2.cc: word is longer than maximum length.\n");
    }
    return k;
  }
  return 0;
}

FILE *openftowrite(const char fname[]){
  FILE *fd;
  if(!(fd=fopen(fname, "w"))){
    fprintf(stderr, "FILE \"%s\" could not be opened for writing. EXITING.\n", fname);exit(0);
  }
  return fd;
} 
FILE *openftoread(const char fname[]){
  FILE *fd;
  if(!(fd=fopen(fname, "r"))){
    fprintf(stderr, "FILE \"%s\" could not be opened for reading. EXITING.\n", fname);exit(0);
  }
  return fd;
} 


int readDataFile(const char fname[], int **data, int max){
  //No error checking
  FILE *fd;
  int lim=1000;
  char oneline[lim];
  char val1[50], val2[50], val3[50];
  int numlines=0, len;

  fd=openftoread(fname);
  while((len = getline(fd, oneline, lim)) > 0 && numlines<max){
    if ((oneline[0] == '#') || (oneline[0] == '/') || (oneline[0] == '\n') || (oneline[0] == '\0')){} // comment/empty lines
    else{
      getnthblock(oneline, val1, 50, 1);
      data[numlines][0]=atoi(val1);
      getnthblock(oneline, val2, 50, 2);
      data[numlines][1]=atoi(val2);
      getnthblock(oneline, val3, 50, 3);
      data[numlines][2]=atoi(val3);
      numlines++;
    }
  }
  if(numlines>=max){
    fprintf(stderr, "Data file \"%s\" too long to be read. EXITING.\n", fname);
    exit(0);
  }

  fclose(fd);

  return numlines;

}


int getoption(char *fname, const char optname[], int num, char v[], int max){
  FILE *fd;
  int len, j, flag=0;
  char oneline[200];
  char modname[50];
  int lim=200;
  fd = openftoread(fname);
  while((len = getline(fd, oneline, lim)) > 0){
    j=0;
    while((isspace((int) oneline[j])) || (oneline[j] == 13)){j++;}
    if ((oneline[j] == '/') || (oneline[j] == '\n') || (oneline[j] == '\0')){} // comment/empty lines
    else{
      getnthblock(oneline, modname, 50, 1);
      if(strcmp(modname, optname) == 0){
        flag = 1;
        getnthblock(oneline, modname, 50, num+1);
        if((int)(strlen(modname)) < max-1)
	  strcpy(v, modname);
        else{
	  fprintf(stderr, "ERROR in routine getoption: Option %s in file %s has value %s which is too long.\n", optname, fname, modname);
	  v[0] = '\0';
	  fclose(fd);
	  return -1;
        }
        break;
      }
    }
  }
  fclose(fd);
  if(flag==0){
    fprintf(stderr, "WARNING in routine getoption: Option %s could not be found in file %s. Setting to default value.\n", optname, fname);
    v[0] = '\0';
    return -2;
  }
  return 0;

}

int getoptioni(char *fname, const char optname[], int defval, FILE *fd1){
  char tempword[200];
  int val;
  if(getoption(fname, optname, 1, tempword, 200)!=0)
    val=defval;
  else
    val=atoi(tempword);
  fprintf(fd1, "#%s %d\n", optname, val);
  return val;
}

int getoption2i(char *fname, const char optname[], int defval, FILE *fd1){
  char tempword[200];
  int val;
  if(getoption(fname, optname, 2, tempword, 200)!=0)
    val=defval;
  else
    val=atoi(tempword);
  fprintf(fd1, "#%s %d\n", optname, val);
  return val;
}

float getoptionf(char *fname, const char optname[], float defval, FILE *fd1){
  char tempword[200];
  float val;
  if(getoption(fname, optname, 1, tempword, 200)!=0)
    val=defval;
  else
    val=atof(tempword);
  fprintf(fd1, "#%s %.4f\n", optname, val);
  return val;
}

float getoption2f(char *fname, const char optname[], float defval, FILE *fd1){
  char tempword[200];
  float val;
  if(getoption(fname, optname, 2, tempword, 200)!=0)
    val=defval;
  else
    val=atof(tempword);
  fprintf(fd1, "#%s %.4f\n", optname, val);
  return val;
}


int randnum(int max){
  return rand()%max;
}

int randpercentage(double perc){// to 1 d.p. Casting to int is flooring
  int intperc=(int)(10.0*perc);
  //fprintf(stderr, "%d\n", intperc);
  if (randnum(1000)<intperc)
    return 1;
  return 0;
}



long factorial(int x){
  int i;
  long factx = 1;
  for(i=1; i<=x ; i++ )
    factx *= i;
  return factx;
}

//binomial distribution shifted to centre at 0
double binom(int n, int param){
  if(param==0){//no distribution
    if(n==0)
      return 1.0;
    else
      return 0.0;
  }
  else if(param==2){//one each side
    if(n==-1 || n==1)
      return 0.25;
    else if(n==0)
      return 0.5;
    else
      return 0.0;
  }
  else if(param==4){//two each side
    if(n==-2 || n==2)
      return 0.0625;
    else if(n==-1 || n==1)
      return 0.25;
    else if(n==0)
      return 0.375;
    else
      return 0.0;
  }
  else if(param==6){//three each side
    if(n==-3 || n==3)
      return 0.015625;
    else if(n==-2 || n==2)
      return 0.09375;
    else if(n==-1 || n==1)
      return 0.234375;
    else if(n==0)
      return 0.3125;
    else
      return 0.0;
  }
  else{
    fprintf(stderr, "ERROR - invalid parameter in binom. EXITING.\n");
    exit(0);
  }
  return 0.0;
}

//Choose from binomial distribution (even parameter up to 6)
int choosefrombin(int param){
  int r;
  int tot;
  if(param==0)
    return 0;
  r=randnum(1000)+1; //1 to 1000
  tot=(int)(1000.0*binom(3,param));
  if(r<tot)
    return -3;
  tot+=(int)(1000.0*binom(3,param));
  if(r<tot)
    return 3;
  tot+=(int)(1000.0*binom(2,param));
  if(r<tot)
    return -2;
  tot+=(int)(1000.0*binom(2,param));
  if(r<tot)
    return 2;
  tot+=(int)(1000.0*binom(1,param));
  if(r<tot)
    return -1;
  tot+=(int)(1000.0*binom(1,param));
  if(r<tot)
    return 1;
  return 0;

}


//
// Gamma distribution
//

double gamma(double shp, double scl, std::default_random_engine & generator)
{
  static std::gamma_distribution<double> dist;
  return dist(generator, std::gamma_distribution<double>::param_type(shp, scl));
}

//
// Uniform distribution
//

double unif(double lend, double rend, std::default_random_engine & generator)
{
  static std::uniform_real_distribution<double> dist1;
  return dist1(generator, std::uniform_real_distribution<double>::param_type(lend, rend));
}

//
// uniform distribution on integers
//

int unifi(int lend, int rend, std::default_random_engine & generator)
{
  static std::uniform_int_distribution<int> dist1;
  return dist1(generator, std::uniform_int_distribution<int>::param_type(lend, rend));
}

//
// normal distribution
//

double norml(double mean, double stdev, std::default_random_engine & generator)
{
  static std::normal_distribution<double> dist1;
  return dist1(generator, std::normal_distribution<double>::param_type(mean, stdev));
}


//in case of gamma distribution
inf::inf(int orgnum, double shp, double scl){
  age = 0;
  ill = 0;
  quar = 0;
  num = orgnum;
  double number = gamma(shp, scl, generator);
  if((numtoinf=int(round(number)))>MAXDISCPROB-1)
    numtoinf=MAXDISCPROB-1;
//fprintf(stderr, "numtoinf[%d]=%d\n", orgnum, numtoinf);
}


// Set the times at which infection occurs: uniform distribution C++ generator
void inf::setinftimes(int rmin, int rmax){
  int i; 
  for(i=0;i<numtoinf;i++){
    inftimes[i]=unifi(rmin, rmax, generator);
    (infnums[inftimes[i]])++;
  }
}

//Set the times at which infection occurs: gamma distribution
void inf::setinftimes(double shp, double scl){
  int i; 
  int num;
  for(i=0;i<numtoinf;i++){
    num = int(round(gamma(shp, scl, generator)));
    if(num>0 && num<MAXAGE)
      inftimes[i]=num;
    else if(num>=MAXAGE)
      inftimes[i]=MAXAGE-1;
    else
      inftimes[i]=1;
    (infnums[inftimes[i]])++;
  }
}

inf **infar(long nl, long nh)
/* allocates a set of pointers to infs */
{
  long nrow = nh-nl+1;
  inf **m;
  m=(inf **) malloc((size_t)((nrow+1)*sizeof(inf*)));
  if (!m) fprintf(stderr, "allocation failure in infar()\n");
  m+=1;
  m-=nl;

  return m;
}

void free_infar(inf **m, long nl, long nh)
/* free an inf array allocated by infar() */
{
  free((char *) (m+nl-1));
}

int **imatrix(long nrl, long nrh, long ncl, long nch)
/* allocate a int matrix with subscript range m[nrl..nrh][ncl..nch] */
{
  long i, nrow=nrh-nrl+1,ncol=nch-ncl+1;
  int **m;

  /* allocate pointers to rows */
  m=(int **) malloc((size_t)((nrow+1)*sizeof(int*)));
  if (!m) fprintf(stderr, "allocation failure 1 in matrix()");
  m += 1;
  m -= nrl;


  /* allocate rows and set pointers to them */
  m[nrl]=(int *) malloc((size_t)((nrow*ncol+1)*sizeof(int)));
  if (!m[nrl]) fprintf(stderr, "allocation failure 2 in matrix()");
  m[nrl] += 1;
  m[nrl] -= ncl;

  for(i=nrl+1;i<=nrh;i++) m[i]=m[i-1]+ncol;

  /* return pointer to array of pointers to rows */
  return m;
}

void free_imatrix(int **m, long nrl, long nrh, long ncl, long nch){
  free((char *) (m[nrl]+ncl-1));
  free((char *) (m+nrl-1));
}

double **dmatrix(long nrl, long nrh, long ncl, long nch)
/* allocate a double matrix with subscript range m[nrl..nrh][ncl..nch] */
{
  long i, nrow=nrh-nrl+1,ncol=nch-ncl+1;
  double **m;

  /* allocate pointers to rows */
  m=(double **) malloc((size_t)((nrow+1)*sizeof(double*)));
  if (!m) fprintf(stderr, "allocation failure 1 in matrix()");
  m += 1;
  m -= nrl;


  /* allocate rows and set pointers to them */
  m[nrl]=(double *) malloc((size_t)((nrow*ncol+1)*sizeof(double)));
  if (!m[nrl]) fprintf(stderr, "allocation failure 2 in matrix()");
  m[nrl] += 1;
  m[nrl] -= ncl;

  for(i=nrl+1;i<=nrh;i++) m[i]=m[i-1]+ncol;

  /* return pointer to array of pointers to rows */
  return m;
}

void free_dmatrix(double **m, long nrl, long nrh, long ncl, long nch){
  free((char *) (m[nrl]+ncl-1));
  free((char *) (m+nrl-1));
}


int nextpos=0;
int create(inf *infs[], int inftype, double alpha, double beta, int inf_gam, int inf_start, int inf_end, double inf_mid, double inf_tm_shp, int *numinf, int *numinf_town, int numinf_village[], int *numinf_red, int *numinf_town_red, int numinf_village_red[], int *numcurinf, int *newinfs, int *newinfs_town, int newinfs_village[], int *numill, double percill, double percdeath, double time_to_death, double dist_on_death, double time_to_recovery, double dist_on_recovery, double time_to_sero, double dist_on_sero, double quardate, double quarp, double dist_on_quardate, double testp, double testdelay, double testdelay_shp, int seromax, double dist_on_seromax, int serofinal, double dist_on_serofinal, int *sero_time, int *sero_max, int *sero_final, int *sero_cur, int *inf_ages){
  int i=nextpos++;
  int j;
  double inf_tm_scl=inf_mid/inf_tm_shp;

  if(nextpos>=MAXINFS){
    fprintf(stderr, "limit MAXINFS reached. You can increase MAXINFS in TownVillage.cc. EXITING.\n");
    exit(0);
  }

  inf_ages[i]=0;
  infs[i] = new inf(i, alpha, beta);
  (*numinf)++;(*numinf_red)++;(*numcurinf)++;(*newinfs)++;
  infs[i]->type=inftype;
  if(inftype==1){//town
    (*numinf_town)++;
    (*numinf_town_red)++;
    (*newinfs_town)++;
  }
  else{//village
    (numinf_village[inftype-2])++;
    (numinf_village_red[inftype-2])++;
    (newinfs_village[inftype-2])++;
  }

  if(dist_on_sero>=0)//discrete simple
    sero_time[i]=(int)time_to_sero+choosefrombin((int)dist_on_sero);
  else//normal dist., -dist_on_sero=stdev
    sero_time[i]=int(round(norml(time_to_sero, -dist_on_sero, generator)));

  sero_cur[i]=0;

  //normal distribution on initial level
  sero_max[i]=int(round(norml((double)seromax, -dist_on_seromax, generator)));

  //normal distribution on final level
  sero_final[i]=int(round(norml((double)serofinal, -dist_on_serofinal, generator)));


  for(j=0;j<MAXAGE;j++){//number to infect at time j
    infs[i]->infnums[j]=0;
  }
  // who falls ill?
  // Currently unused - left in for potential use
  if(randpercentage(percill)){
    if(randpercentage(percdeath)){
      infs[i]->ill=-1;//falls ill and dies
      if(dist_on_death>=0)//discrete simple
	infs[i]->dth_time=(int)time_to_death+choosefrombin((int)dist_on_death);
      else//normally distributed, -dist_on_death=stdev
	infs[i]->dth_time=int(round(norml(time_to_death, -dist_on_death, generator)));

    }
    else{
      infs[i]->ill=1;//falls ill but recovers
      if(dist_on_recovery>=0)
	infs[i]->recov_time=(int)time_to_recovery+choosefrombin((int)dist_on_recovery);
      else{//normal dist, -dist_on_recovery=stdev
	infs[i]->recov_time=int(round(norml(time_to_recovery, -dist_on_recovery, generator)));
	// if(infs[i]->recov_time>MAXAGE)
	//   fprintf(stderr, "recov_time=%d\n", infs[i]->recov_time);
      }
    }
    (*numill)++;
    //fprintf(stderr, "ill=%d\n", infs[i]->ill);
  }
  else{//won't fall ill
    if(dist_on_recovery>=0)
      infs[i]->recov_time=(int)time_to_recovery+choosefrombin((int)dist_on_recovery);
    else//normal dist, -dist_on_recovery=stdev
      infs[i]->recov_time=int(round(norml(time_to_recovery, -dist_on_recovery, generator)));
  }

  infs[i]->quardt=100;infs[i]->testdt=100;//default no quarantining/testing
  if(randpercentage(quarp)){//to quarantine?
    if(dist_on_quardate>=0)
      infs[i]->quardt=(int)quardate+choosefrombin((int)dist_on_quardate);
    else
      infs[i]->quardt=int(round(norml(quardate, -dist_on_quardate, generator)));

    if(randpercentage(testp)){// to test?
      if(testdelay==0 || testdelay_shp<0)
	infs[i]->testdt=infs[i]->quardt + testdelay;//testing on fixed day after quarantine date
      else//testing delay follows a gamma distribution
	infs[i]->testdt=infs[i]->quardt+int(round(gamma(testdelay_shp, testdelay/testdelay_shp, generator)));
    }
  }

  //last operation (one greater than last operation)
  if(infs[i]->ill==-1){//dies (last op. is testing or death)
    infs[i]->lastop_time=infs[i]->dth_time;
    if(infs[i]->testdt!=100 && infs[i]->testdt > infs[i]->lastop_time)
      infs[i]->lastop_time=infs[i]->testdt;
  }
  else{//recovers (last op. is testing, death or seroconversion)
    infs[i]->lastop_time=infs[i]->recov_time;
    if(infs[i]->testdt!=100 && infs[i]->testdt > infs[i]->lastop_time)
      infs[i]->lastop_time=infs[i]->testdt;
    if(sero_time[i] > infs[i]->lastop_time)
      infs[i]->lastop_time=sero_time[i];
  }
  (infs[i]->lastop_time)++;


  //set infection times
  if(inf_gam)//gamma distributed
    infs[i]->setinftimes(inf_tm_shp, inf_tm_scl);
  else
    infs[i]->setinftimes(inf_start, inf_end);
  
  inflist[i]=inftype;
  return i;

}

void die(inf *a){//negate list position and delete
  inflist[a->num]=-(inflist[a->num]);
  delete a;
  return;
}

int intdecay(int max, int min, double halftime, int t){
  double mx=(double)max;
  double mn=(double)min;
  double k=0.69314718/halftime;
  double cur=mn+((mx-mn)*exp(-k*(double)t));
  return int(round(cur));
}




int main(int argc, char *argv[]){
  //for random seeding
  int timeint;
  time_t timepoint;
  int i, ii, tmpi, j, m, r, cur, num_runs;//number of runs
  double R0_town, R0_village, R0_townvillage;
  int flag;
  double trueR0_town,trueR0_village,actualR0;
  int totdays;//total simulation length
  inf **infs=infar(0, MAXINFS-1);
  int init_infs;
  //average time from infection to death, recovery, testing, and seroconversion.
  double avdthtime, avrecovtime, avtesttime, avserotime; 
  int numdeaths, numdeaths_town, numdeaths_village, newdeaths;
  int numrecovs;
  float dthrate_town,dthrate_village;//IFR in towns and villages as a percentage 
  // gamma distribution scale parameter on distribution of individual R0 values
  double infscl_town, infscl_village, infscltmp;//scale for num to infect distribution
  double infshp;//shape for num to infect distribution; assumed same in towns and villages
  int numvillages=100;
  int numinf, numinf_town, numinf_v, *numinf_village;//number of total infections (cumulative)
  int numinf_red, numinf_town_red, numinf_v_red, *numinf_village_red;//numinf after subtracting those who become vulnerable to reinfection
  int reinf_vul_town, reinf_vul_village;//vulnerable to reinfection
  int numcurinf;//number currently infected
  int numinfectious;// number in the infectious window (not used)
  int newinfs,newinfs_town,newinfs_v, *newinfs_village; // number of new infections this time step. 
  int numquar;//number quarantined (cumulative)
  int numtest,numtest_town,numtest_village,newtests;//number tested (cumulative) and new
  int numill;//number ill (cumulative, currently not used)
  int numsero, numsero_town, numsero_village;//cumulative seroconversion figures
  int seromax=1000, serofinal=100;//currently hard-coded
  double dist_on_seromax=-20, dist_on_serofinal=-20;
  double sero_reinfect_mult=2.0;//0=50% chance
  double sero_reinfect=serofinal+sero_reinfect_mult*dist_on_serofinal;
  double sero_ht=30;//half-time for decay of antibodies (days)
  int sero_threshold=200, tmpsero;//threshold for detection
  double percill;//percentage who fall (seriously) ill
  double percdeath_town, percdeath_village, percdeath_tmp;//percentage of ill who die
  //physical distancing?
  int haspd;//boolean
  int pd_at_dth;//pd starts at nth death
  int pd_at_test;//pd starts at nth tested infection
  int pd_at_inf;//pd starts at nth infection
  float pdeff1_town, pdeff1_village, pdeff1_mixed;//effectiveness of physical distancing
  float pdeff_town, pdeff_village, pdeff_mixed;//effectiveness of physical distancing. E.g. 40% - removes 2 in 5 contacts
  int pd;//physical distancing is currently occurring
  int inf_gam;//to gamma distribute infection times or not
  int inf_start, inf_end; //start and end of infective window
  double inf_mid, inf_tm_shp; // mean and shape parameter if gamma distributed
  double time_to_death, time_to_recovery, time_to_sero;//self explanatory
  double dist_on_death, dist_on_recovery, dist_on_sero;//binomial distributions: values 0,2,4,6
  double quarp_town, quarp_village, quarp_tmp;//percentage who get quarantined
  double testp_town, testp_village, testp_tmp;//percentage *of those quarantined* who are tested
  double quardate;//Currently assume all tests occur on a particular day in the infection cycle. Only those tested are quarantined. 
  double dist_on_quardate;//distribution on quardate
  double testdelay, testdelay_shp;


  double totpop, totpop_town, *totpop_village;// total population
  double effpop_town, effpop_v, *effpop_village;// effective population (disease localisation by mitigation)
  double townprop;

  //to keep track of daily transmissions between different populations
  int towntotown=0,towntovillage=0,villagetotown=0,villagetovillage=0;

  int haslockdown;//lockdown?
  int lockdownlen, lockdown2len;//length of lockdowns (used as a term for general mitigation)
  int lockdownday;//days since the start of the first lockdown
  int lockdown2day;//days since the start of the second lockdown
  int lockdown2startday;//days after the start of the first lockdown that the second lockdown start
  float pdeff_lockdown_town, pdeff_lockdown2_town;
  float pdeff_lockdown_village, pdeff_lockdown2_village;
  float pdeff_lockdown_mixed, pdeff_lockdown2_mixed;
  int lockdown_at_dth;//The lockdown begins after the death number lockdown_at_dth. 
  int lockdown_at_test;//The lockdown begins after test number lockdown_at_test.
  int lockdown_at_inf;//The lockdown begins after infection number lockdown_at_inf.

  double popleak_town, popleak_village, popleak2_town, popleak2_village;//leak into effective population post lockdown
  double popleak_frac_town, popleak_frac_village, popleak2_frac_town, popleak2_frac_village;//leak into effective population post lockdown as a fraction of the compartment population

  double popleak_len_town, popleak_len_village, popleak2_len_town, popleak2_len_village;//length of leak into effective population
  // start and end days of leak into town and village populations
  int popleak_start_day_town=0, popleak2_start_day_town=0, popleak_end_day_town, popleak2_end_day_town;
  int popleak_start_day_village=0, popleak2_start_day_village=0, popleak_end_day_village, popleak2_end_day_village;
  float ip_town, ip_village, ip2_town, ip2_village;//infectible proportions at start of first and second lockdown

  // The level of herd immunity in each compartment: depends on effective rather than total populations
  double herdlevel_town=0, *herdlevel_village, hv;
  char paramfilename[200], outfilename[200], logfname[204];
  FILE *fd0, *fd1, *fd2, *fd3; //files to store output
  //FILE *fd3;


  //These parameters are relevant if we want to 
  //run simulations upto or a certain number of days
  //beyond a particular death trigger
  //Currently hard-wired in, to avoid overloading parameter files
  int topresent=0;//only simulate to a fixed day namely "presentday" days after "trigger_dths" deaths or "trigger_infs" infections
  int trigger_dths=1;//Number of deaths which trigger the clock. Not for synchronisation
  int trigger_infs=1000;//Number of infections which trigger the clock. Not for synchronisation
  int startinfs, endinfs;
  int presentday=1;//Number of days to run after trigger
  int startclock=0;//The clock

  //for doubling times
  int totdoubling=0;
  double avdoubling=0;
  double avinfs, avdths;//average infections and deaths at trigger point
  char endfname[206];
  double *town_IR, *village_IR, *IR;
  double town_IR_av=0, village_IR_av=0, IR_av=0;


  if(argc < 2){
    fprintf(stderr, "ERROR: you must provide a parameter file name. You may also provide an output file name.\n");
    exit(0);
  }
  strncpy (paramfilename, argv[1], sizeof(paramfilename));
  if(argc>=3)
    strncpy (outfilename, argv[2], sizeof(outfilename));
  else
    strcpy(outfilename, "output/outfile");//default output file

  fd1=openftowrite(outfilename); //tab separated output
  strcpy(endfname, outfilename);strcat(endfname, ".csv");
  fd2=openftowrite(endfname);
  strcpy(endfname, outfilename);strcat(endfname, "1.csv");
  fd3=openftowrite(endfname);

  strcpy(logfname, outfilename);strcat(logfname, "_log");
  fd0=openftowrite(logfname); //log file

  //options: general
  num_runs=getoptioni(paramfilename, "number_of_runs", 10, fd1);//model runs
  numvillages=getoptioni(paramfilename, "numvillages", 100, fd1);//number of villages
  townprop=getoptionf(paramfilename, "townprop", 0.5, fd1);//fraction of total pop in town
  dthrate_town=getoptionf(paramfilename, "death_rate_town", 0.5, fd1);//death rate
  dthrate_village=getoptionf(paramfilename, "death_rate_village", 0.5, fd1);//death rate
  R0_town=getoptionf(paramfilename, "R0_town", 5.0, fd1);//basic reproduction number town (approximately)
  R0_village=getoptionf(paramfilename, "R0_village", 2.8, fd1);//basic reproduction number village (approximately)
  R0_townvillage=getoptionf(paramfilename, "R0_townvillage", 0.8, fd1);
  infshp=getoptionf(paramfilename, "infshp", 0.1, fd1);//shape param
  totdays=getoptioni(paramfilename, "totdays", 150, fd1);//total simulation length
  totpop=getoptionf(paramfilename, "population", 13000000, fd1);//population

  //totpop=MAXINFS;
  inf_gam=getoptioni(paramfilename, "inf_gam", 0, fd1);//use gamma distribution for infection times? Default is no
  inf_start=getoptioni(paramfilename, "inf_start", 2, fd1);//start of infective window
  inf_end=getoptioni(paramfilename, "inf_end", 9, fd1);//end of infective window
  // if infection times are gamma distributed
  inf_mid=getoptionf(paramfilename, "inf_mid", 6, fd1);//mean infection time
  inf_tm_shp=getoptionf(paramfilename, "inf_tm_shp", 4, fd1);//shape parameter for infection time

  time_to_death=getoptionf(paramfilename, "time_to_death", 17, fd1);//survival time
  dist_on_death=getoptionf(paramfilename, "dist_on_death", -3, fd1);//distribution on time_to_death. Default = none
  time_to_recovery=getoptionf(paramfilename, "time_to_recovery", 20, fd1);//recovery time
  dist_on_recovery=getoptionf(paramfilename, "dist_on_recovery", -2, fd1);//distribution on time_to_recovery
  time_to_sero=getoptionf(paramfilename, "time_to_sero", 14, fd1);//seroconversion time
  dist_on_sero=getoptionf(paramfilename, "dist_on_sero", -3, fd1);//distribution on time_to_sero
  sero_reinfect_mult=getoptionf(paramfilename, "sero_reinfect_mult", 5.0, fd1);
  sero_reinfect=serofinal+sero_reinfect_mult*dist_on_serofinal;

  init_infs=getoptioni(paramfilename, "initial_infections", 10, fd1);//initial number infected
  //options: quarantine and testing
  quarp_town=getoptionf(paramfilename, "percentage_quarantined_town", 4, fd1);//percentage of infecteds who are quarantined
  quarp_village=getoptionf(paramfilename, "percentage_quarantined_village", 4, fd1);//percentage of infecteds who are quarantined
  testp_town=getoptionf(paramfilename, "percentage_tested_town", 100, fd1);//the percentage *of those quarantined* who are tested
  testp_village=getoptionf(paramfilename, "percentage_tested_village", 100, fd1);//the percentage *of those quarantined* who are tested
  quardate=getoptionf(paramfilename, "quardate", 12, fd1);//mean date of testing and quarantining
  dist_on_quardate=getoptionf(paramfilename, "dist_on_quardate", -3, fd1);//distribution on quarantine date
  testdelay=getoptionf(paramfilename, "testdelay", 0, fd1);//mean delay from quarantining to testing
  testdelay_shp=getoptionf(paramfilename, "testdelay_shp", -1, fd1);//distribution on delay between quarantining and testing
  //options: lockdown
  haslockdown=getoptioni(paramfilename, "haslockdown", 0, fd1);//lockdown?

  //options: lockdown 1
  lockdown_at_dth=getoptioni(paramfilename, "lockdown_at_dth", -1, fd1);//lockdown at nth death
  lockdown_at_test=getoptioni(paramfilename, "lockdown_at_test", -1, fd1);//lockdown at nth test
  lockdown_at_inf=getoptioni(paramfilename, "lockdown_at_inf", -1, fd1);//lockdown at nth test
  lockdownlen=getoptioni(paramfilename, "lockdownlen", 0, fd1);//length of lockdown
  ip_town=getoptionf(paramfilename, "infectible_proportion_town", 0.05555, fd1);
  ip_village=getoptionf(paramfilename, "infectible_proportion_village", 0.05555, fd1);
  pdeff_lockdown_town=getoptionf(paramfilename, "pdeff_lockdown_town", 60, fd1);
  pdeff_lockdown_village=getoptionf(paramfilename, "pdeff_lockdown_village", 60, fd1);
  pdeff_lockdown_mixed=getoptionf(paramfilename, "pdeff_lockdown_mixed", 60, fd1);
  popleak_frac_town=getoptionf(paramfilename, "popleak_frac_town", 0, fd1);
  popleak_frac_village=getoptionf(paramfilename, "popleak_frac_village", 0, fd1);
  totpop_town=townprop*totpop;
  popleak_town=0.01*popleak_frac_town*totpop_town;
  popleak_len_town=getoptioni(paramfilename, "popleak_len_town", 0, fd1);
  popleak_len_village=getoptioni(paramfilename, "popleak_len_village", 0, fd1);
  popleak_start_day_town=getoptioni(paramfilename, "popleak_start_day_town", 0, fd1);
  popleak_end_day_town=popleak_start_day_town+popleak_len_town-1;
  popleak_start_day_village=getoptioni(paramfilename, "popleak_start_day_village", 0, fd1);
  popleak_end_day_village=popleak_start_day_village+popleak_len_village-1;

  //options: lockdown 2
  if(haslockdown==2){
    lockdown2startday=getoptioni(paramfilename, "lockdown2startday", 0, fd1);//start day of second lockdown
    lockdown2len=getoption2i(paramfilename, "lockdownlen", 0, fd1);//length of lockdown
    ip2_town=getoption2f(paramfilename, "infectible_proportion_town", 0.05555, fd1);
    ip2_village=getoption2f(paramfilename, "infectible_proportion_village", 0.05555, fd1);
 
    pdeff_lockdown2_town=getoption2f(paramfilename, "pdeff_lockdown_town", 60, fd1);
    pdeff_lockdown2_village=getoption2f(paramfilename, "pdeff_lockdown_village", 60, fd1);
    pdeff_lockdown2_mixed=getoption2f(paramfilename, "pdeff_lockdown_mixed", 60, fd1);
    popleak2_frac_town=getoption2f(paramfilename, "popleak_frac_town", 0, fd1);
    popleak2_frac_village=getoption2f(paramfilename, "popleak_frac_village", 0, fd1);
    popleak2_town=0.01*popleak2_frac_town*totpop_town;

    popleak2_len_town=getoption2i(paramfilename, "popleak_len_town", 0, fd1);
    popleak2_len_village=getoption2i(paramfilename, "popleak_len_village", 0, fd1);
    popleak2_start_day_town=getoption2i(paramfilename, "popleak_start_day_town", 0, fd1);
    popleak2_end_day_town=popleak2_start_day_town+popleak2_len_town-1;

    popleak2_start_day_village=getoption2i(paramfilename, "popleak_start_day_village", 0, fd1);
    popleak2_end_day_village=popleak2_start_day_village+popleak2_len_village-1;

  }

  //options: physical distancing
  haspd=getoptioni(paramfilename, "physical_distancing", 0, fd1);//physical distancing?

  pd_at_dth=getoptioni(paramfilename, "pd_at_dth", -1, fd1);//physical distancing at nth death
  pd_at_test=getoptioni(paramfilename, "pd_at_test", -1,fd1);//physical distancing at nth recorded infection
  pd_at_inf=getoptioni(paramfilename, "pd_at_inf", -1,fd1);//physical distancing at nth infection
  pdeff1_town=getoptionf(paramfilename, "pdeff1_town", 30, fd1);//effectiveness of physical distancing
  pdeff1_village=getoptionf(paramfilename, "pdeff1_village", 30, fd1);//effectiveness of physical distancing
  pdeff1_mixed=getoptionf(paramfilename, "pdeff1_mixed", 30, fd1);//effectiveness of physical distancing

  numinf_village=(int *) malloc((size_t)(numvillages*sizeof(int)));
  numinf_village_red=(int *) malloc((size_t)(numvillages*sizeof(int)));
  newinfs_village=(int *) malloc((size_t)(numvillages*sizeof(int)));
  totpop_village=(double *) malloc((size_t)(numvillages*sizeof(double)));
  effpop_village=(double *) malloc((size_t)(numvillages*sizeof(double)));
  herdlevel_village=(double *) malloc((size_t)(numvillages*sizeof(double)));
  town_IR=(double *) malloc((size_t)(num_runs*sizeof(double)));
  village_IR=(double *) malloc((size_t)(num_runs*sizeof(double)));
  IR=(double *) malloc((size_t)(num_runs*sizeof(double)));


  for(ii=0;ii<numvillages;ii++)
    herdlevel_village[ii]=0;


  for(ii=0;ii<numvillages;ii++)
    totpop_village[ii]=(totpop-totpop_town)/numvillages;

  popleak_village=0.01*popleak_frac_village*totpop_village[0];
  if(haslockdown==2)
    popleak2_village=0.01*popleak2_frac_village*totpop_village[0];


  //gamma distribution on individual R0 values
  infscl_town=R0_town/infshp;infscl_village=R0_village/infshp;

  //fd3=fopen("data1/graph", "w");

  percill=20.0;//percentage of people who fall quite ill (not currently used - for hospitalisations data?)
  percdeath_town=dthrate_town*100.0/percill;
  percdeath_village=dthrate_village*100.0/percill;

  //random seeding
  timeint = time(&timepoint); /*convert time to an integer */
  srand(timeint);
  generator.seed(timeint);//seeding for distribution generator

  trueR0_town=0;trueR0_village=0;

  for(i=0;i<1000000;i++){
    trueR0_town+=round(gamma(infshp, infscl_town, generator))/1000000.0;
    trueR0_village+=round(gamma(infshp, infscl_village, generator))/1000000.0;
  }
  fprintf(fd0, "trueR0_town=%.4f, trueR0_village=%.4f\n", trueR0_town, trueR0_village);
  fprintf(fd0, "run\tsteps\tactualR0\tavdthtime\tavrecovtime\tavtesttime\tavserotime\ttown_IR\tvillage_IR\tIR\n");

  //nest order: For each run... for each day... for each individual
  avinfs=0.0;avdths=0.0;
  for(r=0;r<num_runs;r++){//Each model run
    startclock=0;nextpos=0;
    numinf=0;numinf_town=0;numinf_v=0;numinf_red=0;numinf_town_red=0;numinf_v_red=0;
    numcurinf=0;numdeaths=0;numdeaths_town=0;numdeaths_village=0;newdeaths=0;numrecovs=0;
    numquar=0;numtest=0;numtest_town=0;numtest_village=0;newtests=0;numill=0;numsero=0;numsero_town=0;numsero_village=0;
    actualR0=0;avdthtime=0;avrecovtime=0;avserotime=0;avtesttime=0;
    lockdownday=0;lockdown2day=0;
    effpop_town=totpop_town;
    for(i=0;i<numvillages;i++){
      numinf_village[i]=0;
      numinf_village_red[i]=0;
      effpop_village[i]=totpop_village[i];
    }
    reinf_vul_town=0;reinf_vul_village=0;
   
    pd=0;

    for(i=0;i<MAXINFS;i++){inflist[i]=0;sero_time[i]=-1;sero_max[i]=-1;sero_final[i]=-1;sero_cur[i]=-1;inf_ages[i]=-1;}//initialise

    for(i=0;i<init_infs;i++){//all town to start with
      cur=create(infs, 1, infshp, infscl_village, inf_gam, inf_start, inf_end, inf_mid, inf_tm_shp, &numinf, &numinf_town, numinf_village, &numinf_red, &numinf_town_red, numinf_village_red, &numcurinf, &newinfs, &newinfs_town, newinfs_village, &numill, percill, percdeath_village, time_to_death, dist_on_death, time_to_recovery, dist_on_recovery, time_to_sero, dist_on_sero, quardate, quarp_village, dist_on_quardate, testp_village, testdelay, testdelay_shp, seromax, dist_on_seromax, serofinal, dist_on_serofinal, sero_time, sero_max, sero_final, sero_cur, inf_ages);

      //fprintf(fd3, "0 %d\n", cur);

      actualR0=actualR0*((double)(numinf-1))/((double)(numinf))+(double)((infs[cur])->numtoinf)/((double)(numinf));

      //fprintf(stderr, "actualR0=%.4f\n", actualR0);
      //fprintf(stderr, "infs[%d] (illstate=%d) will infect %d at times:\n",cur, infs[cur]->ill, infs[cur]->numtoinf);
      // for(j=0;j<(infs[cur])->numtoinf;j++){
      // 	fprintf(stderr, "   %d\n", infs[cur]->inftimes[j]);
      // }
    }

    
    for(m=0;m<totdays;m++){//each day

      numinfectious=0;newinfs=0;newinfs_town=0;newdeaths=0;newtests=0;
      for(ii=0;ii<numvillages;ii++)
	newinfs_village[ii]=0;

      towntotown=0;towntovillage=0;villagetotown=0;villagetovillage=0;
      if(haspd && ((pd_at_dth>0 && numdeaths>=pd_at_dth) || (pd_at_test>0 && numtest>=pd_at_test) || (pd_at_inf>0 && numinf>=pd_at_inf))){//physical distancing
	pd=1;
	pdeff_town=pdeff1_town;
	pdeff_village=pdeff1_village;
	pdeff_mixed=pdeff1_mixed;
      }
      else
	pd=0;

      if(haslockdown){
	//lockdown 2 overrides if the two overlap
	if(haslockdown==2 && lockdownday>=lockdown2startday && lockdown2day<lockdown2len){
	  if(lockdown2day==0){
	    effpop_town=totpop_town*ip2_town;//effective infectible town population
	    effpop_v=0;
	    for(i=0;i<numvillages;i++){
	      effpop_village[i]=totpop_village[i]*ip2_village;//effective infectible village population
	      effpop_v+=effpop_village[i];
	    }
	    //@@fprintf(stderr, "\nLockdown 2 starts. Effective population now %.0f(towns), %.0f(villages).\n", effpop_town, effpop_v);
	  }
	  else{ 
	    if(lockdown2day>=popleak2_start_day_town && lockdown2day<=popleak2_end_day_town){
	      effpop_town+=popleak2_town;
	    }
	    if(lockdown2day>=popleak2_start_day_village && lockdown2day<=popleak2_end_day_village){
	      effpop_v=0;
	      for(i=0;i<numvillages;i++){
		effpop_village[i]+=popleak2_village;
		effpop_v+=effpop_village[i];
	      }
	    }
	    //@@fprintf(stderr, "\nIn lockdown 2. Effective population now %.0f(towns), %.0f(villages).\n", effpop_town, effpop_v);
	  }
	  pd=1;
	  pdeff_town=pdeff_lockdown2_town;
	  pdeff_village=pdeff_lockdown2_village;
	  pdeff_mixed=pdeff_lockdown2_mixed;
	  lockdown2day++;
	}
	else if(((lockdown_at_dth>0 && numdeaths>=lockdown_at_dth) || (lockdown_at_test>0 && numtest>=lockdown_at_test) || (lockdown_at_inf>0 && numinf>=lockdown_at_inf)) && lockdownday<lockdownlen){
	  if(lockdownday==0){
	    effpop_v=0;
	    for(i=0;i<numvillages;i++){
	      effpop_village[i]=totpop_village[i]*ip_village;
	      effpop_v+=effpop_village[i];
	    }
	    effpop_town=totpop_town*ip_town;
	    
	    fprintf(stderr, "\nLockdown 1 starts. Effective population now %.0f(towns), %.0f(villages).\n", effpop_town, effpop_v);
	  }
	  else{ 
	    if(lockdownday>=popleak_start_day_town && lockdownday<=popleak_end_day_town){
	      effpop_town+=popleak_town;
	    }
	    if(lockdownday>=popleak_start_day_village && lockdownday<=popleak_end_day_village){
	      effpop_v=0;
	      for(i=0;i<numvillages;i++){
		effpop_village[i]+=popleak_village;
		effpop_v+=effpop_village[i];
	      }
	    }
	    fprintf(stderr, "\nIn lockdown 1. Effective population now %.0f(towns), %.0f(villages).\n", effpop_town, effpop_v);
	  }
	  pd=1;
	  pdeff_town=pdeff_lockdown_town;
	  pdeff_village=pdeff_lockdown_village;
	  pdeff_mixed=pdeff_lockdown_mixed;
	  lockdownday++;
	}
	else{//lockdown finishes. Assume physical distancing returns to early levels
	  effpop_town=totpop_town;
	  effpop_v=0;
	  for(i=0;i<numvillages;i++){
	    effpop_village[i]=totpop_village[i];
	    effpop_v+=effpop_village[i];
	  }
	  if(lockdownday>=lockdownlen){
	    fprintf(stderr, "Lockdown finished. Effective population now %.0f(towns), %.0f(villages).\n", effpop_town, effpop_v);
	    lockdownday++;//to know when to enter lockdown2
	  }
	  if(haspd){
	    pdeff_town=pdeff1_town;
	    pdeff_village=pdeff1_village;
	    pdeff_mixed=pdeff1_mixed;
	  }
	  
	}
      }
      if(pd){
	fprintf(stderr, "physical distancing = %.2f(towns), %.2f(villages), %.2f(mixed).\n", pdeff_town, pdeff_village, pdeff_mixed);
      }

      for(i=0;i<MAXINFS;i++){//for each infected person
	if(inf_ages[i]>=0){//still being processed
	  (inf_ages[i])++;
	  if(sero_time[i]>0){//decay of antibodies
	    tmpsero=sero_cur[i];
	    sero_cur[i]=intdecay(sero_max[i], sero_final[i], sero_ht, inf_ages[i]-sero_time[i]);
	    //detection threshold
	    if(tmpsero>=sero_threshold && sero_cur[i]<sero_threshold){//crossed below the threshold
	      numsero--;//fprintf(stderr, "*");
	      if(abs(inflist[i])==1)
		numsero_town--;
	      else
		numsero_village--;
	    }
	    //susceptibility to reinfection threshold
	    if(tmpsero>=sero_reinfect && sero_cur[i]<sero_reinfect){//crossed below the reinfection threshold
	      if(abs(inflist[i])==1){//decrease prior infection levels in towns
		numinf_town_red--;
		reinf_vul_town++;
	      }
	      else{
		numinf_village_red[abs(inflist[i])]--;
		reinf_vul_village++;
	      }
	      numinf_red--;
	    }
	  }
	}


	if(inflist[i]>=1){
	  (infs[i]->age)++;//age updates at start...
	  if(infs[i]->age==infs[i]->lastop_time){//done with
	    die(infs[i]);//deallocate
	    continue;
	  }

	  if(infs[i]->age >= inf_start && infs[i]->age <= inf_end){//so far kept as is regardless of distribution
	    numinfectious++;
	  }

	  if(infs[i]->age==sero_time[i]){//seroconversion
	    sero_cur[i]=sero_max[i];
	    if(sero_cur[i]>=sero_threshold){
	      numsero++;
	      if(infs[i]->type==1)
		numsero_town++;
	      else
		numsero_village++;
	    }
	    avserotime=avserotime*((double)(numsero-1))/((double)(numsero))+(double)((infs[i])->age)/((double)(numsero));
	  }


	  if(infs[i]->age==infs[i]->quardt){//quarantine?
	    infs[i]->quar=1;
	    numquar++;
	  }

	  if(infs[i]->age==infs[i]->testdt){//test?
	    numtest++;newtests++;
	    if(infs[i]->type==1)
	      numtest_town++;
	    else
	      numtest_village++;
	    avtesttime=avtesttime*((double)(numtest-1))/((double)(numtest))+(double)((infs[i])->age)/((double)(numtest));
	  }

	  if(infs[i]->ill==-1 && infs[i]->age==infs[i]->dth_time){//die
	    numdeaths++;newdeaths++;numcurinf--;
	    if(infs[i]->type==1)
	      numdeaths_town++;
	    else
	      numdeaths_village++;
	    avdthtime=avdthtime*((double)(numdeaths-1))/((double)(numdeaths))+(double)((infs[i])->age)/((double)(numdeaths));
	  }
	  else if(infs[i]->ill!=-1 && infs[i]->age==infs[i]->recov_time){//recover
	    numcurinf--;numrecovs++;
	    avrecovtime=avrecovtime*((double)(numrecovs-1))/((double)(numrecovs))+(double)((infs[i])->age)/((double)(numrecovs));
	  }
	  else if(infs[i]->quar==0 && infs[i]->age<MAXAGE){//still being processed, not quarantined
	    herdlevel_town=100.0*((double)numinf_town_red/(double)effpop_town);
	    for(ii=0;ii<numvillages;ii++){
	      herdlevel_village[ii]=100.0*((double)numinf_village_red[ii]/(double)effpop_village[ii]);
	    }
            for(j=0;j<infs[i]->infnums[infs[i]->age];j++){
	      flag=0;
	      //4 cases town-town, town-village, village-town, village-village
	      if(infs[i]->type==1){//town infector
		if(randpercentage(R0_townvillage/R0_town*100.0)){//would interact with village infectee
		  if(!pd||randpercentage(100.0-pdeff_mixed)){//not wiped out by town-village physical distancing
		    ii=randnum(numvillages);
		    if(randpercentage(100.0-herdlevel_village[ii])){//infection actually occurs
		      flag=ii+2;towntovillage++;
		      infscltmp=infscl_village;
		      quarp_tmp=quarp_village;
		      testp_tmp=testp_village;
		      percdeath_tmp=percdeath_village;
		    }
		  }
		}
		else{//interacts with town infectee
		  if(!pd||randpercentage(100.0-pdeff_town)){//not wiped out by town-town physical distancing
		    if(randpercentage(100.0-herdlevel_town)){//infection actually occurs
		      flag=1;towntotown++;
		      infscltmp=infscl_town;
		      quarp_tmp=quarp_town;
		      testp_tmp=testp_town;
		      percdeath_tmp=percdeath_town;
		    }
		  }
		}
	      }
	      else{//village infector
		if(randpercentage(R0_townvillage/R0_village*100.0)){//would interact with town infectee
		  if(!pd||randpercentage(100.0-pdeff_mixed)){//not wiped out by town-village physical distancing
		    if(randpercentage(100.0-herdlevel_town)){//infection actually occurs
		      flag=1;villagetotown++;
		      infscltmp=infscl_town;
		      quarp_tmp=quarp_town;
		      testp_tmp=testp_town;
		      percdeath_tmp=percdeath_town;
		    }
		  }
		}
		else{//interacts with village infectee
		  if(!pd||randpercentage(100.0-pdeff_village)){//not wiped out by village-village physical distancing
		    if(randpercentage(100.0-herdlevel_village[infs[i]->type-2])){//infection actually occurs
		      flag=infs[i]->type;villagetovillage++;//within village
		      infscltmp=infscl_village;
		      quarp_tmp=quarp_village;
		      testp_tmp=testp_village;
		      percdeath_tmp=percdeath_village;
		    }
		  }
		}
	      }

	      if(flag){
		tmpi=create(infs, flag, infshp, infscltmp, inf_gam, inf_start, inf_end, inf_mid, inf_tm_shp, &numinf, &numinf_town, numinf_village, &numinf_red, &numinf_town_red, numinf_village_red, &numcurinf, &newinfs, &newinfs_town, newinfs_village, &numill, percill, percdeath_tmp, time_to_death, dist_on_death, time_to_recovery, dist_on_recovery, time_to_sero, dist_on_sero, quardate, quarp_tmp, dist_on_quardate, testp_tmp, testdelay, testdelay_shp, seromax, dist_on_seromax, serofinal, dist_on_serofinal, sero_time, sero_max, sero_final, sero_cur, inf_ages);
		actualR0=actualR0*((double)(numinf-1))/((double)(numinf))+(double)((infs[tmpi])->numtoinf)/((double)(numinf));
		//this will update to zero as they come later in the sequence
		infs[tmpi]->age--;
	      }
	    }
	  }
	}
      }//cycled through all infected individuals

      numinf_v=0;numinf_v_red=0;newinfs_v=0;
      for(ii=0;ii<numvillages;ii++){
	numinf_v+=numinf_village[ii];
	numinf_v_red+=numinf_village_red[ii];
	newinfs_v+=newinfs_village[ii];
      }

  

      //fprintf(fd2, "%d: numinf(town,village)=%d(%d,%d)\n", m, numinf, numinf_town, numinf_v);
      //fprintf(fd2, "villages: ");
      //fprintf(stderr, "%d,%d,", m, numinf_town);
      //for(ii=0;ii<numvillages;ii++){
	//fprintf(stderr, "%d,", numinf_village[ii]);
      //}
      //fprintf(stderr, "\n");
      fprintf(fd3, "%d,%.4f,%.4f,%d,", m, (double)numinf_town/(double)totpop_town,(double)numinf_v/(double)(totpop-totpop_town),numinf_town);
      for(ii=0;ii<numvillages;ii++){
	fprintf(fd3, "%d,", numinf_village[ii]);
      }
      fprintf(fd3, "\n");
      fprintf(fd2, "%d,%.4f,%.4f,%.4f,%d,", m, 100*(double)numinf_town/(double)effpop_town, (double)newinfs_town/(double)totpop_town,(double)newinfs_v/(double)(totpop-totpop_town),newinfs_town);
      for(ii=0;ii<numvillages;ii++){
	fprintf(fd2, "%d,", newinfs_village[ii]);
      }
      fprintf(fd2, "\n");

      // if(newinfs>0)
      // 	fprintf(stderr, "towntotown=%.4f\ttowntovillage=%.4f\tvillagetotown=%.4f\tvillagetovillage=%.4f\treinf_vul_town=%d\treinf_vul_village=%d\n",(double)towntotown/(double)newinfs*100.0,(double)towntovillage/(double)newinfs*100.0,(double)villagetotown/(double)newinfs*100.0,(double)villagetovillage/(double)newinfs*100.0,reinf_vul_town,reinf_vul_village);

      //current infection rates in town, village and combined
      hv=0;
      for(ii=0;ii<numvillages;ii++)
	hv+=((double)numinf_village[ii]);
      town_IR[r]=100.0*((double)numinf_town/(double)totpop_town);
      village_IR[r]=100.0*hv/(double)(totpop-totpop_town);
      IR[r]=100.0*((double)numinf_town+hv)/totpop;

      fprintf(stderr, "%d,town_IR=%.4f, village_IR=%.4f, IR=%.4f\n", m, town_IR[r], village_IR[r], IR[r]);
 

      fprintf(fd1,"%d\t%d\t%d\t%d\t %d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", m, numinf, newinfs, numcurinf, numdeaths, newdeaths, numtest,newtests,numinfectious,numsero,newinfs_town,newinfs_v);


      fflush(fd0);fflush(fd1);

      //Are we running the model only to a particular moment?
      if(topresent){
	if(trigger_infs){//triggered by infection numbers
	  if(numinf>=trigger_infs){
	    //printf("%d, %d: numinf=%d, numdeaths=%d, \n", r, m, numinf, numdeaths);
	    if(startclock==0)
	      startinfs=numinf;
	    startclock++;
	  }
	  if(startclock==presentday+1){
	    //printf("%d, %d: numinf=%d, numdeaths=%d, \n", r, m, numinf, numdeaths);
	    endinfs=numinf;
	    printf("model run %d: %d %d %d\n", r, startinfs, endinfs, presentday);
	    if(presentday>0 && endinfs-startinfs!=0){
	      printf("doubling=%.4f\n", log(2.0)*(presentday)/(log(endinfs)-log(startinfs)));
	      totdoubling++; avdoubling+=log(2.0)*(presentday)/(log(endinfs)-log(startinfs));
	    }
	    avinfs+=numinf;avdths+=numdeaths;
	    break;
	  }
	}
	else{
	  if(numdeaths>=trigger_dths){
	    if(startclock==0)
	      startinfs=numinf;
	    startclock++;
	  }
	  
	  if(startclock==presentday+1){
	    endinfs=numinf;
	    printf("model run %d: %d %d %d\n", r, startinfs, endinfs, presentday);
	    if(presentday>0 && endinfs-startinfs!=0){
	      printf("doubling=%.4f\n", log(2.0)*(presentday)/(log(endinfs)-log(startinfs)));
	      totdoubling++; avdoubling+=log(2.0)*(presentday)/(log(endinfs)-log(startinfs));
	    }
	    avinfs+=numinf;avdths+=numdeaths;
	    break;
	  }
	}
      }
    }

    fprintf(fd1,"\n");

    town_IR_av+=town_IR[r];village_IR_av+=village_IR[r];IR_av+=IR[r];
    for(i=0;i<MAXINFS;i++){//free memory
      if(inflist[i]>=1)
	die(infs[i]);//deallocate (numcurinf will get reset anyway)
    }
    fprintf(fd0, "%d\t%d\t%.4f\t%.4f\t%.4f\t%.4f\t%.4f\t%.4f\t%.4f\t%.4f\n", r+1, m, actualR0, avdthtime, avrecovtime, avtesttime, avserotime, town_IR[r], village_IR[r], IR[r]);
  }

  town_IR_av/=(double)num_runs;village_IR_av/=(double)num_runs;IR_av/=(double)num_runs;
  fprintf(stderr, "town_IR_av=%.4f, village_IR_av=%.4f, IR_av=%.4f\n", town_IR_av, village_IR_av, IR_av);


  if(topresent){
    if(totdoubling>0)
      printf("avinfs=%.4f, avdeaths=%.4f, av. doubling time=%.4f\n", avinfs/((double)num_runs), avdths/((double)num_runs),avdoubling/((double)totdoubling));
    else
      printf("avinfs=%.4f, avdeaths=%.4f\n", avinfs/((double)num_runs), avdths/((double)num_runs));
  }

  free_infar(infs, 0, MAXINFS-1);
  fclose(fd0);fclose(fd1);fclose(fd2);fclose(fd3);
  free((char*)numinf_village);free((char*)numinf_village_red);free((char*)newinfs_village);free((char*)totpop_village);free((char*)effpop_village);free((char*)herdlevel_village);free((char*)town_IR);free((char*)village_IR);free((char*)IR);
  return 0;
}

