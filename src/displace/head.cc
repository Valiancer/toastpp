#include <fstream>
#include "mathlib.h"
#include "felib.h"

using namespace std;

int main (void)
{
    double tol = 1e-8;

    Mesh mesh;
    int *rowptr, *colidx, nzero, niter;
    int i, j, cmd, reg, el, idx, nd, dofnod, totdof, nloads, *nf;
    double load, E, nu, rad;
    char meshname[256];
    bool freenodes;

    cout << "Mesh name: ";
    cin  >> meshname;
    ifstream mesh_f (meshname);
    mesh_f >> mesh;
    mesh.Setup();
    int n = mesh.nlen();

    ifstream test_f ("test.dat");
    test_f >> dofnod;
    test_f >> E;
    test_f >> nu;

    bool *rest = new bool[n*dofnod];
    for (i = 0; i < n*dofnod; i++) rest[i] = false;

    cout << "Fix boundary (1|0)? ";
    cin >> cmd;
    if (cmd) {
        for (i = 0; i < n; i++)
	    if (mesh.nlist[i].isBnd())
	        for (j = 0; j < dofnod; j++) rest[i*dofnod+j] = true;
    }
    do {
        cout << "Fix region (1|0)? ";
	cin >> cmd;
	if (cmd) {
	    cout << "Region id: ";
	    cin >> reg;
	    for (i = 0; i < n; i++)
	        if (mesh.nlist[i].Region() == reg)
		    for (j = 0; j < dofnod; j++) rest[i*dofnod+j] = true;
	}
    } while (cmd);

    cout << "Remove constraints around node (1|0)? ";
    cin >> cmd;
    if (cmd) {
        cout << "Node: ";
	cin >> nd;
	cout << "Radius: ";
	cin >> rad;
        for (i = 0; i < n; i++) {
	    double d = Dist (mesh.nlist[nd], mesh.nlist[i]);
	    if (d < rad) 
	        for (j = 0; j < dofnod; j++) rest[i*dofnod+j] = false;
	}
    }

    totdof = MakeNodalFreedomArray (nf, n, dofnod, rest);
    RVector rhs(totdof);

    cout << "Load node: " << endl;
    cin >> nd;
    cout << "Load values: " << endl;
    for (j = 0; j < dofnod; j++) {
        cin >> load;
        idx = nf[nd*dofnod+j];
	if (idx >= 0) rhs[idx] = load;
    }

#ifdef UNDEF
    test_f >> nloads;
    for (i = 0; i < nloads; i++) {
        test_f >> nd;
	for (j = 0; j < dofnod; j++) {
	    test_f >> load;
	    idx = nf[nd*dofnod+j];
	    if (idx >= 0) rhs[idx] = load;
	}
    }
#endif

    rowptr = new int[totdof+1];
    colidx = new int[totdof*totdof];
    for (i = 0; i <= totdof; i++) rowptr[i] = i*totdof;
    for (i = 0; i < totdof; i++)
        for (j = 0; j < totdof; j++) colidx[i*totdof+j] = j;

    RCompRowMatrix A (totdof, totdof, rowptr, colidx);
    RCompRowMatrix L (totdof, totdof, rowptr, colidx);
    RVector d (totdof);
    RVector sol(totdof);

    AddElasticStrainDisplacementToSysMatrix (mesh, A, E, nu, nf);
    //A.Print (cout);
    
    RPreconditioner *precon = new RPrecon_Diag;
    precon->Reset (&A);
    niter = CG (A, rhs, sol, tol, precon, 2*n);
    cout << "Solution after " << niter << " CG iterations:\n";
    delete precon;

#ifdef UNDEF
    cout << "\nEquilibrium displacements (u,v):\n";
    cout << "node\tu-value\tv-value\n";
    for (i = 0; i < n; i++) {
        double u, v;
        j = nf[i*dofnod];   u = (j >= 0 ? sol[j] : 0.0);
	j = nf[i*dofnod+1]; v = (j >= 0 ? sol[j] : 0.0);
        cout << i << '\t' << u << '\t' << v << endl;
    }
    
    cout << "\nNode placements (x,y):\n";
    cout << "node\tu-value\tv-value\n";
#endif

#ifdef UNDEF
    ofstream ofs1 ("test.out");
    for (i = 0; i < n; i++) {
        double u, v, x, y;
        j = nf[i*dofnod];   u = (j >= 0 ? sol[j] : 0.0); x = mesh.nlist[i][0];
	j = nf[i*dofnod+1]; v = (j >= 0 ? sol[j] : 0.0); y = mesh.nlist[i][1];
        cout << i << '\t' << x+u << '\t' << y+v << endl;
	ofs1 << x+u << ' ' << y+v << endl;
    }
#endif

    for (i = 0; i < n; i++) {
        double u, v, x, y;
        j = nf[i*dofnod];   u = (j >= 0 ? sol[j] : 0.0); mesh.nlist[i][0] += u;
	j = nf[i*dofnod+1]; v = (j >= 0 ? sol[j] : 0.0); mesh.nlist[i][1] += v;
    }

    cout << "output mesh: ";
    cin >> meshname;
    ofstream mout (meshname);
    mout << mesh;

#ifdef UNDEF
    ofstream ofs2 ("test.plot");
    ofs2 << "plot \"test.out\" with points\n";
    for (el = 0; el < mesh.elen(); el++) {
        Element *pel = mesh.elist[el];
	for (i = 0; i < pel->nSide(); i++) {
	    double u, v, x1, y1, x2, y2;
	    int n1 = pel->Node[pel->SideNode (i,0)];
	    int n2 = pel->Node[pel->SideNode (i,1)];
	    j = nf[n1*dofnod];   u = (j >= 0 ? sol[j] : 0.0);
	    j = nf[n1*dofnod+1]; v = (j >= 0 ? sol[j] : 0.0);
	    x1 = mesh.nlist[n1][0] + u;
	    y1 = mesh.nlist[n1][1] + v;
	    j = nf[n2*dofnod];   u = (j >= 0 ? sol[j] : 0.0);
	    j = nf[n2*dofnod+1]; v = (j >= 0 ? sol[j] : 0.0);
	    x2 = mesh.nlist[n2][0] + u;
	    y2 = mesh.nlist[n2][1] + v;
	    ofs2 << "set arrow from " << x1 << ',' << y1
		 << " to " << x2 << ',' << y2 << " nohead\n";
	}
    }
    ofs2 << "replot\n";
    ofs2 << "pause -1 \"Hit Enter to continue\"\n";
#endif

    return 0;
}
