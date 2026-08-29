/* --------------------------------------------------------------------  */
/*                          CALCULIX                                     */
/*                   - GRAPHICAL INTERFACE -                             */
/*                                                                       */
/*     A 3-dimensional pre- and post-processor for finite elements       */
/*              Copyright (C) 1996 Klaus Wittig                          */
/*                                                                       */
/*     This program is free software; you can redistribute it and/or     */
/*     modify it under the terms of the GNU General Public License as    */
/*     published by the Free Software Foundation; version 2 of           */
/*     the License.                                                      */
/*                                                                       */
/*     This program is distributed in the hope that it will be useful,   */
/*     but WITHOUT ANY WARRANTY; without even the implied warranty of    */ 
/*     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the      */
/*     GNU General Public License for more details.                      */
/*                                                                       */
/*     You should have received a copy of the GNU General Public License */
/*     along with this program; if not, write to the Free Software       */
/*     Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.         */
/* --------------------------------------------------------------------  */

#ifndef CGXCDSM_H
#define CGXCDSM_H

#ifdef __cplusplus
extern "C" {
#endif
#include <cgx.h>
#ifdef __cplusplus
}
#endif

#endif
#undef max
#undef min
#undef NODES
#undef MESH
#undef OUTSIDE
#undef PI

#include "tetgen/tetgen.h"
#include <iostream>

extern double     gtol;
extern char       printFlag;                   /* printf 1:on 0:off */

extern Scale      scale[1];
extern Summen     anz[1];
extern Nodes      *node;
extern Faces      *face;
extern Elements   *e_enqire;

extern SumGeo     anzGeo[1];
extern Gbod       *body;
extern Gsur       *surf;
extern Sets       *set;
extern Sets       *setx;

using namespace std;


int generateTetFromSet(int setNr, double teth, int eattr, int mesherFlag )
{
  int e,i,j,k,n;
  int snodSet, np, ne, cgxtet[10], sumtri3=0, sumtri6=0, n1, n2, nm;
  int *ebuf;
  static int *cgxnode=NULL;
  static int *ngnode=NULL;

  typedef struct {
    int sum, *n2, *nm;
  } N1nm;
  N1nm *n1nm=NULL;

  int nodseq_tr6[]={0,3,1,1,4,2,2,5,0};
  int nodseq_te10[]={0,4,1,1,5,2,2,6,0, 0,7,3,1,8,3,2,9,3};

  setx=NULL;

  // check the consistency of the surface mesh. Either all tr6 or tr3
  // generate a list of surface nodes which will be passed to TetGen
  delSet("+snodSet");
  delSet("+velemSet");
  snodSet=pre_seta("+snodSet","i",0);
  for (i = 0; i < set[setNr].anz_e; i++)
  {
    if (e_enqire[set[setNr].elem[i]].type == 7) sumtri3++;
    else if (e_enqire[set[setNr].elem[i]].type == 8) sumtri6++;
    else
    {
      printf("ERROR: mesh in set %s contains not only triangle elements:%d\n",set[setNr].name, set[setNr].elem[i]);
      return(0);
    }
    for(k=0; k<3; k++) seta(snodSet,"n",e_enqire[set[setNr].elem[i]].nod[k]);
  }
  if((sumtri3)&&(sumtri3!=set[setNr].anz_e))
  {
    printf("ERROR: mesh in set %s contains not only tr3 elements\n",set[setNr].name);
    return(0);
  }
  if((sumtri6)&&(sumtri6!=set[setNr].anz_e))
  {
    printf("ERROR: mesh in set %s contains not only tr6 elements\n",set[setNr].name);
    return(0);
  }

  // add the nodes and elements to mesh
  if ((ngnode = (int *)realloc((int *)ngnode, (anz->nmax+1)*sizeof(int)) ) == NULL )
    { errMsg("ERROR: realloc failure in generateTet\n"); return(0); }

  // Construct in-memory TetGen input structure
  tetgenio in, out;
  in.firstnumber = 1;
  in.numberofpoints = set[snodSet].anz_n;
  in.pointlist = new REAL[in.numberofpoints * 3];

  for (i = 0; i < set[snodSet].anz_n; i++)
  {
    int nid = set[snodSet].node[i];
    ngnode[nid] = i + 1;
    in.pointlist[i * 3 + 0] = node[nid].nx;
    in.pointlist[i * 3 + 1] = node[nid].ny;
    in.pointlist[i * 3 + 2] = node[nid].nz;
  }

  in.numberoffacets = set[setNr].anz_e;
  in.facetlist = new tetgenio::facet[in.numberoffacets];
  for (i = 0; i < set[setNr].anz_e; i++)
  {
    tetgenio::facet *f = &in.facetlist[i];
    f->numberofpolygons = 1;
    f->polygonlist = new tetgenio::polygon[1];
    f->numberofholes = 0;
    f->holelist = NULL;
    tetgenio::polygon *p = &f->polygonlist[0];
    p->numberofvertices = 3;
    p->vertexlist = new int[3];
    for (k = 0; k < 3; k++)
      p->vertexlist[k] = ngnode[e_enqire[set[setNr].elem[i]].nod[k]];
  }

  // Run in-memory tetrahedralization
  char switches[256];
  if (teth > 0.0 && teth < 1e5)
  {
    double maxvol = (teth/scale->w)*(teth/scale->w)*(teth/scale->w)/5.;
    sprintf(switches, "pq1.3/17O7a%e", maxvol);
  }
  else
  {
    sprintf(switches, "pq1.3/17O7");
  }

  printf(" Starting in-memory TetGen mesher (switches: %s)...\n", switches);
  try {
    tetrahedralize(switches, &in, &out);
  }
  catch (int errcode) {
    printf("ERROR in TetGen: execution failed with error code %d\n\n", errcode);
    return 0;
  }
  catch (...) {
    printf("ERROR in TetGen: unknown exception caught during tetrahedralization\n\n");
    return 0;
  }

  if (out.numberoftetrahedra <= 0)
  {
    printf("ERROR: No tetrahedral elements were generated\n\n");
    return 0;
  }

  // define new nodes and tets
  np = out.numberofpoints;
  if ((cgxnode = (int *)realloc((int *)cgxnode, (np+1)*sizeof(int)) ) == NULL )
    { errMsg("ERROR: realloc failure in generateTet\n"); return(0); }
  for (i = 0; i < np; i++)
  {
    if (i < set[snodSet].anz_n)
      cgxnode[i+1] = set[snodSet].node[i];
    else
    {
      cgxnode[i+1] = anz->nnext++;
      nod( anz, &node, 1, cgxnode[i+1], out.pointlist[i*3+0], out.pointlist[i*3+1], out.pointlist[i*3+2], 0 );     
    } 
  }
  delSet( set[snodSet].name );

  /* create a table for all nodes which points to already created midside nodes, surface-elements must still exist */
  if(sumtri6)
  {
    if ( (n1nm = (N1nm *)malloc( (anz->nmax+1) * sizeof(N1nm))) == NULL )
    { printf("\n\n ERROR in mids: malloc\n\n") ; exit(-1); }    
    for (i=0; i<=anz->nmax; i++) n1nm[i].sum=0;
    for (i=0; i<=anz->nmax; i++) n1nm[i].n2=n1nm[i].nm=NULL;
    for (k = 0; k < set[setNr].anz_e; k++)
    {
      for (n=0; n<3; n++)
      {
        n1=e_enqire[set[setNr].elem[k]].nod[nodseq_tr6[n*3]];
        n2=e_enqire[set[setNr].elem[k]].nod[nodseq_tr6[n*3+2]];

        /* check if the nm exists already */
        nm=-1;
        for(i=0; i<n1nm[n1].sum; i++) if(n1nm[n1].n2[i]==n2) nm=n1nm[n1].nm[i];
        for(i=0; i<n1nm[n2].sum; i++) if(n1nm[n2].n2[i]==n1) nm=n1nm[n2].nm[i];

        if(nm==-1)
        {
          nm=e_enqire[set[setNr].elem[k]].nod[nodseq_tr6[n*3+1]];

          if ( (n1nm[n1].n2 = (int *)realloc( n1nm[n1].n2, (n1nm[n1].sum+1) * sizeof(int))) == NULL )
          { printf("\n\n ERROR in mids: realloc\n\n") ; exit(-1); }    
          if ( (n1nm[n1].nm = (int *)realloc( n1nm[n1].nm, (n1nm[n1].sum+1) * sizeof(int))) == NULL )
          { printf("\n\n ERROR in mids: realloc\n\n") ; exit(-1); }    
          n1nm[n1].n2[n1nm[n1].sum]=n2;
          n1nm[n1].nm[n1nm[n1].sum]=nm;
          n1nm[n1].sum++;
        }
      }
    }
  }

  // remove surface elements from surfaces and sets except setNr
  for (k=0; k<anzGeo->s; k++)
  {  
    for (n=0; n<surf[k].ne; n++)
    {
      for (j=0; j<set[setNr].anz_e; j++)
      {
        if(surf[k].elem[n]==set[setNr].elem[j])
        {
          face[ face[surf[k].elem[n]].indx[1] ].nr=-1;
          surf[k].elem[n]=0;
        }
      }
    }
    e=0;
    for(n=0; n<surf[k].ne; n++) if(surf[k].elem[n]>0) surf[k].elem[e++]=surf[k].elem[n];
    surf[k].ne=e; 
  }

  for (k=0; k<anz->sets; k++)
  {
    if(( set[k].name != (char *)NULL )&&( k != setNr))
    {
      if(set[k].anz_e<=0) continue;
      if( (ebuf=(int *)calloc((anz->emax+1), sizeof(int) ) )==NULL) 
        { printf(" ERROR: calloc failure\n"); return(0); }
      for(j=0; j<set[k].anz_e; j++) ebuf[set[k].elem[j]]=1;
      if(set[k].type==0)
      {
        n=set[k].elem[set[k].anz_e-1]; // max elemnr in sorted set
        for(j=0; j<set[setNr].anz_e; j++) { if(set[setNr].elem[j]<=n) ebuf[set[setNr].elem[j]]=0; else break; }
      }
      else { for(j=0; j<set[setNr].anz_e; j++) ebuf[set[setNr].elem[j]]=0; } 
      e=0;
      for(j=0; j<set[k].anz_e; j++) if(ebuf[set[k].elem[j]]>0) set[k].elem[e++]=set[k].elem[j];
      set[k].anz_e=e;
      free(ebuf);
    }
  }

  ne = out.numberoftetrahedra;
  i=0;
  for (j = 0; j < ne; j++)
  {
    cgxtet[0] = cgxnode[out.tetrahedronlist[j * out.numberofcorners + 0]];
    cgxtet[1] = cgxnode[out.tetrahedronlist[j * out.numberofcorners + 1]];
    cgxtet[2] = cgxnode[out.tetrahedronlist[j * out.numberofcorners + 2]];
    cgxtet[3] = cgxnode[out.tetrahedronlist[j * out.numberofcorners + 3]];

    if(i<set[setNr].anz_e) elem_define(anz,&e_enqire, set[setNr].elem[i], 3, cgxtet, 1, eattr );
    else { elem_define(anz,&e_enqire, anz->enext++, 3, cgxtet, 1, eattr ); seta( setNr, "e", anz->emax ); }
    i++;
  }
  ne = i;

  // delete remaining surface elements
  if (i<set[setNr].anz_e) delElem( set[setNr].anz_e-i, &set[setNr].elem[i] ) ;

  /* generate midside nodes */
  if(sumtri6)
  {
    fixMidsideNodes( set[setNr].name, "gen" );

    /* change coords of surface-midside nodes */
    snodSet=pre_seta("+snodSet","i",0);
    for (k = 0; k < set[setNr].anz_e; k++)
    {
      for (n=0; n<6; n++)
      {
        n1=e_enqire[set[setNr].elem[k]].nod[nodseq_te10[n*3]];
        n2=e_enqire[set[setNr].elem[k]].nod[nodseq_te10[n*3+2]];

        /* check if the nm is known */
        nm=-1;
        for(i=0; i<n1nm[n1].sum; i++) if(n1nm[n1].n2[i]==n2) nm=n1nm[n1].nm[i];
        for(i=0; i<n1nm[n2].sum; i++) if(n1nm[n2].n2[i]==n1) nm=n1nm[n2].nm[i];

        if(nm!=-1)
        {
          /* change node */
          seta(snodSet,"n",e_enqire[set[setNr].elem[k]].nod[nodseq_te10[n*3+1]]); 
          e_enqire[set[setNr].elem[k]].nod[nodseq_te10[n*3+1]]=nm;
        }
      }
    }
    zap( set[snodSet].name );
    fixMidsideNodes( set[setNr].name, "" );
  }

  printf("tet-mesh done with h:%f (generated %d tetrahedra, %d nodes)\n", teth, ne, np);
  return(ne);
}



int generateTetFromBody(int nr, double teth, int eattr, int mesherFlag)
{
  int i,j,k,n,s,sb;
  int setNr, snodSet, snodSet2, mnodSet, np, ne, cgxtet[10], sumtri=0, sumtri3=0, sumtri6=0, n1, n2, nm;
  static int *cgxnode=NULL;
  static int *ngnode=NULL;
  int tryToFlipBody=0;

  typedef struct {
    int sum, *n2, *nm;
  } N1nm;
  N1nm *n1nm=NULL;

  int nodseq_tr6[]={0,3,1,1,4,2,2,5,0};
  int nodseq_te10[]={0,4,1,1,5,2,2,6,0, 0,7,3,1,8,3,2,9,3};

  setx=NULL;

  // check the consistency of the surface mesh. Either all tr6 or tr3
  delSet("+snodSet");
  delSet("+mnodSet");
  delSet("+velemSet");
  snodSet=pre_seta("+snodSet","i",0);
  mnodSet=pre_seta("+mnodSet","i",0);
  setNr=pre_seta("+velemSet","i",0);
  for(sb=0; sb<body[nr].ns; sb++)
  {
    s=body[nr].s[sb];
    for (i = 0; i < surf[s].ne; i++)
    {
      sumtri++;
      if (e_enqire[surf[s].elem[i]].type == 7) sumtri3++;
      else if (e_enqire[surf[s].elem[i]].type == 8)
      {
        sumtri6++;
        for(k=0; k<3; k++) seta(mnodSet,"n",e_enqire[surf[s].elem[i]].nod[k+3]);
      }
      else
      {
        printf("ERROR: mesh in surf %s contains not only triangle elements:%d\n",surf[s].name, surf[s].elem[i]);
        return(0);
      }
      for(k=0; k<3; k++) seta(snodSet,"n",e_enqire[surf[s].elem[i]].nod[k]);
    }
  }
  if((sumtri3)&&(sumtri3!=sumtri))
  {
    printf("ERROR: surface-mesh in body %s contains not only tr3 elements\n",body[nr].name);
    return(0);
  }
  if((sumtri6)&&(sumtri6!=sumtri))
  {
    printf("ERROR: surface-mesh in body %s contains not only tr6 elements\n",body[nr].name);
    return(0);
  }

  // add the nodes and elements to mesh
  if ((ngnode = (int *)realloc((int *)ngnode, (anz->nmax+1)*sizeof(int)) ) == NULL )
    { errMsg("ERROR: realloc failure in generateTet\n"); return(0); }

tryToFlipBodyMark:;

  // Construct in-memory TetGen input structure
  tetgenio in, out;
  in.firstnumber = 1;
  in.numberofpoints = set[snodSet].anz_n;
  in.pointlist = new REAL[in.numberofpoints * 3];

  for (i = 0; i < set[snodSet].anz_n; i++)
  {
    int nid = set[snodSet].node[i];
    ngnode[nid] = i + 1;
    in.pointlist[i * 3 + 0] = node[nid].nx;
    in.pointlist[i * 3 + 1] = node[nid].ny;
    in.pointlist[i * 3 + 2] = node[nid].nz;
  }

  n=0;
  for(sb=0; sb<body[nr].ns; sb++) n+=surf[body[nr].s[sb]].ne;
  in.numberoffacets = n;
  in.facetlist = new tetgenio::facet[in.numberoffacets];

  int facet_idx = 0;
  for(sb=0; sb<body[nr].ns; sb++)
  {
    s=body[nr].s[sb];
    n=1;
    if(body[nr].o[sb]=='-') n*=-1;
    if(body[nr].ori=='-') n*=-1;
    if(tryToFlipBody) n*=-1;

    for (i = 0; i < surf[s].ne; i++)
    {
      tetgenio::facet *f = &in.facetlist[facet_idx++];
      f->numberofpolygons = 1;
      f->polygonlist = new tetgenio::polygon[1];
      f->numberofholes = 0;
      f->holelist = NULL;
      tetgenio::polygon *p = &f->polygonlist[0];
      p->numberofvertices = 3;
      p->vertexlist = new int[3];

      if(n==-1)
      {
        p->vertexlist[0] = ngnode[e_enqire[surf[s].elem[i]].nod[2]];
        p->vertexlist[1] = ngnode[e_enqire[surf[s].elem[i]].nod[1]];
        p->vertexlist[2] = ngnode[e_enqire[surf[s].elem[i]].nod[0]];
      }
      else
      {
        p->vertexlist[0] = ngnode[e_enqire[surf[s].elem[i]].nod[0]];
        p->vertexlist[1] = ngnode[e_enqire[surf[s].elem[i]].nod[1]];
        p->vertexlist[2] = ngnode[e_enqire[surf[s].elem[i]].nod[2]];
      }
    }
  }

  // Run in-memory tetrahedralization
  char switches[256];
  if (teth > 0.0 && teth < 1e5)
  {
    // Global max volume constraint
    double maxvol = (teth/scale->w)*(teth/scale->w)*(teth/scale->w)/5.;
    sprintf(switches, "pq1.3/17O7a%e", maxvol);
  }
  else
  {
    // Pure boundary-driven graded Delaunay refinement
    sprintf(switches, "pq1.3/17O7");
  }

  printf(" Starting in-memory TetGen mesher for body %s (switches: %s)...\n", body[nr].name, switches);
  bool meshing_ok = true;
  try {
    tetrahedralize(switches, &in, &out);
  }
  catch (int errcode) {
    meshing_ok = false;
  }
  catch (...) {
    meshing_ok = false;
  }

  if (!meshing_ok || out.numberoftetrahedra <= 0)
  {
    if (tryToFlipBody == 0)
    {
      printf(" Notice: Retrying body %s with flipped surface orientation...\n", body[nr].name);
      tryToFlipBody = 1;
      goto tryToFlipBodyMark;
    }
    else
    {
      printf("ERROR: TetGen failed to mesh body %s\n\n", body[nr].name);
      return 0;
    }
  }

  // define new nodes and tets
  np = out.numberofpoints;
  if ((cgxnode = (int *)realloc((int *)cgxnode, (np+1)*sizeof(int)) ) == NULL )
    { errMsg("ERROR: realloc failure in generateTet\n"); return(0); }
  for (i = 0; i < np; i++)
  {
    if(i<set[snodSet].anz_n) cgxnode[i+1]=set[snodSet].node[i];
    else
    {
      cgxnode[i+1]=anz->nnext++;
      nod( anz, &node, 1, cgxnode[i+1], out.pointlist[i*3+0], out.pointlist[i*3+1], out.pointlist[i*3+2], 0 );     
    } 
  }

  /* create a table for all nodes which points to already created midside nodes, surface-elements must still exist */
  if(sumtri6)
  {
    if ( (n1nm = (N1nm *)malloc( (anz->nmax+1) * sizeof(N1nm))) == NULL )
    { printf("\n\n ERROR in mids: malloc\n\n") ; exit(-1); }    
    for (i=0; i<=anz->nmax; i++) n1nm[i].sum=0;
    for (i=0; i<=anz->nmax; i++) n1nm[i].n2=n1nm[i].nm=NULL;
    for(sb=0; sb<body[nr].ns; sb++)
    {
      s=body[nr].s[sb];
      for (k = 0; k < surf[s].ne; k++)
      {
        for (n=0; n<3; n++)
        {
          n1=e_enqire[surf[s].elem[k]].nod[nodseq_tr6[n*3]];
          n2=e_enqire[surf[s].elem[k]].nod[nodseq_tr6[n*3+2]];
    
          /* check if the nm exists already */
          nm=-1;
          for(i=0; i<n1nm[n1].sum; i++) if(n1nm[n1].n2[i]==n2) nm=n1nm[n1].nm[i];
          for(i=0; i<n1nm[n2].sum; i++) if(n1nm[n2].n2[i]==n1) nm=n1nm[n2].nm[i];
    
          if(nm==-1)
          {
            nm=e_enqire[surf[s].elem[k]].nod[nodseq_tr6[n*3+1]];
    
            if ( (n1nm[n1].n2 = (int *)realloc( n1nm[n1].n2, (n1nm[n1].sum+1) * sizeof(int))) == NULL )
            { printf("\n\n ERROR in mids: realloc\n\n") ; exit(-1); }    
            if ( (n1nm[n1].nm = (int *)realloc( n1nm[n1].nm, (n1nm[n1].sum+1) * sizeof(int))) == NULL )
            { printf("\n\n ERROR in mids: realloc\n\n") ; exit(-1); }    
            n1nm[n1].n2[n1nm[n1].sum]=n2;
            n1nm[n1].nm[n1nm[n1].sum]=nm;
            n1nm[n1].sum++;
          }
        }
      }
    }
  }

  ne = out.numberoftetrahedra;

  /* allocate memory for embeded elements */
  if((body[nr].elem=(int *)realloc((int *)body[nr].elem, (ne)*sizeof(int)) )==NULL)
  { printf(" ERROR: realloc failure in generateTet body:%s can not be meshed\n\n" , body[nr].name); return(0); }
  if((body[nr].nod=(int *)realloc((int *)body[nr].nod, (ne)*sizeof(int)) )==NULL)
  { printf(" ERROR: realloc failure in generateTet body:%s can not be meshed\n\n" , body[nr].name); return(0); }

  i=0;
  for (j = 0; j < ne; j++)
  {
    cgxtet[0] = cgxnode[out.tetrahedronlist[j * out.numberofcorners + 0]];
    cgxtet[1] = cgxnode[out.tetrahedronlist[j * out.numberofcorners + 1]];
    cgxtet[2] = cgxnode[out.tetrahedronlist[j * out.numberofcorners + 2]];
    cgxtet[3] = cgxnode[out.tetrahedronlist[j * out.numberofcorners + 3]];

    body[nr].elem[i]=anz->enext;
    elem_define(anz,&e_enqire, anz->enext++, 3, cgxtet, 1, eattr );
    seta( setNr, "e", anz->emax );
    i++;
  }
  ne = i;
  body[nr].ne=ne;

  /* generate midside nodes */
  if(sumtri6)
  {
    fixMidsideNodes( set[setNr].name, "gen" );
    /* change coords of surface-midside nodes */
    snodSet2=pre_seta("+snodSet2","i",0);

    for (k = 0; k < set[setNr].anz_e; k++)
    {
      for (n=0; n<6; n++)
      {
        n1=e_enqire[set[setNr].elem[k]].nod[nodseq_te10[n*3]];
        n2=e_enqire[set[setNr].elem[k]].nod[nodseq_te10[n*3+2]];

        /* check if the nm is known */
        nm=-1;
        for(i=0; i<n1nm[n1].sum; i++) if(n1nm[n1].n2[i]==n2) nm=n1nm[n1].nm[i];
        for(i=0; i<n1nm[n2].sum; i++) if(n1nm[n2].n2[i]==n1) nm=n1nm[n2].nm[i];

        if(nm!=-1)
        {
          /* change node */
          seta(snodSet2,"n",e_enqire[set[setNr].elem[k]].nod[nodseq_te10[n*3+1]]); 
          e_enqire[set[setNr].elem[k]].nod[nodseq_te10[n*3+1]]=nm;
        }
      }
    }

    /* add body-nodes */
    for (k = 0; k < set[setNr].anz_e; k++)
      for (n=0; n<10; n++) seta( setNr, "n", e_enqire[set[setNr].elem[k]].nod[n] );

    zap( set[snodSet2].name );
    fixMidsideNodes( set[setNr].name, "" );
  }
  else
  {
    /* add body-nodes */
    for (k = 0; k < set[setNr].anz_e; k++)
      for (n=0; n<4; n++) seta( setNr, "n", e_enqire[set[setNr].elem[k]].nod[n] );
  }

  /* remove surface nodes */
  for (k = 0; k < set[snodSet].anz_n; k++) setr(setNr,"n",set[snodSet].node[k]);
  for (k = 0; k < set[mnodSet].anz_n; k++) setr(setNr,"n",set[mnodSet].node[k]);

  /* add to body */
  if((body[nr].nod=(int *)realloc((int *)body[nr].nod, (set[setNr].anz_n+1)*sizeof(int)) )==NULL)
  { printf(" ERROR: realloc failure in generateTet body:%s can not be meshed\n\n" , body[nr].name); return(0); }
  for (k = 0; k < set[setNr].anz_n; k++) body[nr].nod[k]=set[setNr].node[k];
  body[nr].nn=set[setNr].anz_n;

  delSet("+snodSet");
  delSet("+mnodSet");
  delSet("+velemSet");

  printf("tet-mesh done with h:%f (generated %d tetrahedra, %d nodes for body %s)\n", teth, ne, np, body[nr].name);
  return(ne);
}
