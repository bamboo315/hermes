#define HERMES_REPORT_WARN
#define HERMES_REPORT_INFO
#define HERMES_REPORT_VERBOSE
#define HERMES_REPORT_FILE "application.log"
#include "hermes1d.h"

//  This example solves the 1D heat transfer equation 
//
//  PDE: du/dt - Laplace u - f = 0.
//
//  Interval: (A, B).
//
//  DC: Dirichlet, u(A) = u(B) = 0
//
//  IC: u(x, 0) = 0 for all x
//
//  The following parameters can be changed:

const int NEQ = 1;                      // Number of equations.
const int NELEM = 30;                   // Number of elements.
const double A = 0, B = 2*M_PI;         // Domain end points.
const int P_INIT = 3;                   // Polynomial degree.
const double TAU = 0.1;                 // Time step.
const double T_FINAL = 1.0;             // Length of time interval.

// Newton's method.
double NEWTON_TOL = 1e-5;               // Tolerance.
int NEWTON_MAX_ITER = 150;              // Max. number of Newton iterations.

MatrixSolverType matrix_solver = SOLVER_UMFPACK;  // Possibilities: SOLVER_AMESOS, SOLVER_AZTECOO, SOLVER_MUMPS,
                                                  // SOLVER_PETSC, SOLVER_SUPERLU, SOLVER_UMFPACK.

// Boundary conditions.
BCSpec DIR_BC_LEFT(0, 0.0);
BCSpec DIR_BC_RIGHT(0, 0.0);

// Function f(x).
double f(double x) 
{
  return 1.0;
}

// Weak forms for Jacobi matrix and residual.
#include "definitions.cpp"

int main() 
{
  // Create space, set Dirichlet BC, enumerate basis functions.
  Space* space = new Space(A, B, NELEM, DIR_BC_LEFT, DIR_BC_RIGHT, P_INIT, NEQ);
  int ndof = Space::get_num_dofs(space);
  info("ndof: %d", ndof);

  // Initialize the weak formulation.
  WeakForm wf;
  wf.add_matrix_form(jacobian);
  wf.add_vector_form(residual);

  // Initialize the FE problem.
  DiscreteProblem *dp = new DiscreteProblem(&wf, space);

  // Allocate coefficient vector.
  double *coeff_vec = new double[ndof];
  memset(coeff_vec, 0, ndof*sizeof(double));

  // Set up the solver, matrix, and rhs according to the solver selection.
  SparseMatrix* matrix = create_matrix(matrix_solver);
  Vector* rhs = create_vector(matrix_solver);
  Solver* solver = create_linear_solver(matrix_solver, matrix, rhs);

  // Time stepping loop.
  double current_time = 0.0;
  while (current_time < T_FINAL) 
  {
    // Newton's loop.
    // Fill vector coeff_vec using dof and coeffs arrays in elements.
    get_coeff_vector(space, coeff_vec);

    int it = 1;
    while (true) 
    {
      // Assemble the Jacobian matrix and residual vector.
      dp->assemble(coeff_vec, matrix, rhs);

      // Calculate the l2-norm of residual vector.
      double res_l2_norm = get_l2_norm(rhs);

      // Info for user.
      info("---- Newton iter %d, ndof %d, res. l2 norm %g", it, Space::get_num_dofs(space), res_l2_norm);

      // If l2 norm of the residual vector is within tolerance, then quit.
      // NOTE: at least one full iteration forced
      //       here because sometimes the initial
      //       residual on fine mesh is too small.
      if(res_l2_norm < NEWTON_TOL && it > 1) break;

      // Multiply the residual vector with -1 since the matrix 
      // equation reads J(Y^n) \deltaY^{n+1} = -F(Y^n).
      for(int i=0; i<ndof; i++) rhs->set(i, -rhs->get(i));

      // Solve the linear system.
      if(!solver->solve())
	error ("Matrix solver failed.\n");

      // Add \deltaY^{n+1} to Y^n.
      for (int i = 0; i < ndof; i++) coeff_vec[i] += solver->get_solution()[i];

      // If the maximum number of iteration has been reached, then quit.
      if (it >= NEWTON_MAX_ITER) error ("Newton method did not converge.");

      // Copy coefficients from vector y to elements.
      set_coeff_vector(coeff_vec, space);

      it++;
    }

    // Plot the solution.
    Linearizer l(space);
    char filename[100];
    sprintf(filename, "solution_%g.gp", current_time);
    l.plot_solution(filename);
    info("Solution %s written.", filename);

    current_time += TAU;
  }

  // Plot the resulting space.
  space->plot("space.gp");

  // Cleaning
  delete dp;
  delete rhs;
  delete solver;
  delete[] coeff_vec;
  delete space;
  delete matrix;

  info("Done.");
  return 0;
}

