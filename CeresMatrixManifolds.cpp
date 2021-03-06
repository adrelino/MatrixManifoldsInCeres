﻿
#include "Parameterizations.hpp"
#include "ceres/gradient_problem.h"
#include "ceres/gradient_problem_solver.h"

using namespace ceres;

// find a matrix X that is closest to a given matrix A in the Frob. sense: argmin_X ||X-A||_{fro}
// In a Euclidean Manifold X=A. 
class MatrixDenoising : public FirstOrderFunction {
public:
	MatrixDenoising(MatrixXd &_A)
		: A(_A) { };
	virtual ~MatrixDenoising() {}

	virtual bool Evaluate(const double* parameters,
		double* cost, double* gradient) const {

		const Map<const MatrixXd> P0(parameters, A.rows(), A.cols());

		if (gradient == NULL)
			cost[0] = costFrobenius(P0);
		else
		{
			Map<MatrixXd> eGrad(gradient, A.rows(), A.cols());
			cost[0] = costGradientFrobenius(P0, eGrad);
		}

		return true;
	}

	virtual int NumParameters() const {
		return A.rows()*A.cols();
	}

	double costFrobenius(const MatrixXd &X) const
	{
		double cost = (X - A).norm();
		return 0.5*cost;
	}

	MatrixXd gradientFrobenius(const MatrixXd &X) const
	{
		return X - A;
	}
	
	template <typename T>
	double costGradientFrobenius(const MatrixXd &X, T& Grad) const
	{
		Grad = gradientFrobenius(X);
		double cost = Grad.norm();
		return 0.5*cost;
	}

	MatrixXd &A;
};

GradientProblemSolver::Summary solveGradientProblem(GradientProblem &gradientProblem, MatrixXd& solution)
{
	GradientProblemSolver::Options options;
	options.max_num_iterations = 200;
	options.logging_type = ceres::LoggingType::PER_MINIMIZER_ITERATION; // default: ceres::PER_MINIMIZER_ITERATION
	options.minimizer_progress_to_stdout = true;
	options.function_tolerance = 1e-8; // some early stopping?
	options.gradient_tolerance = 1e-8;
	options.line_search_direction_type = ceres::LineSearchDirectionType::LBFGS; // make sure we use LBFGS here
	options.line_search_type = ceres::LineSearchType::WOLFE; // make sure we use WOLFE when LBFGS is used 
															 // options.max_num_line_search_step_size_iterations = 10;
															 // options.max_num_line_search_direction_restarts = 100;

	GradientProblemSolver::Summary summary;
	ceres::Solve(options, gradientProblem, (double*)solution.data(), &summary);

	return summary;
}

// doubly stochastic denoising
int main_DS_denoise()
{
	size_t N = 10;
	MatrixXd A = (MatrixXd)(MatrixXd::Random(N, N)); // just a random matrix
	A = A.cwiseAbs(); // make it non-negative.
	MatrixXd X = DSn_rand(N); // A random matrix on Birkhoff - initial solution
	
	cout << "Given Matrix:\n" << A << endl << endl;  // print the matrix
	cout << "Initial Solution:\n" << X << endl << endl;  // print the initial solution

	// create our first order gradient problem
	MatrixDenoising *denoiseFunction = new MatrixDenoising(A);
	BirkhoffParameterization *birkhoffParameterization = new BirkhoffParameterization(N);
	GradientProblem birkhoffProblem(denoiseFunction, birkhoffParameterization);

	GradientProblemSolver::Summary summary = solveGradientProblem(birkhoffProblem, X);

	cout<<summary.FullReport()<<endl;

	// check if X is on the manifold
	cout << "Final Solution:\n" << X << endl << endl;  // print the final solution
	cout << "Is X on Manifold: "<<DSn_check(X, 0.0001) << endl;

    return 0;
}

// stiefel denoising
int main()
{
	size_t N = 10, K = 10; // specific case when N=K : orthogonal group
	MatrixXd A = (MatrixXd)(MatrixXd::Random(N, K)); // just a random matrix
	MatrixXd X = Stiefel_rand(N, K); // A random matrix on Stiefel manifold - initial solution

	cout << "Given Matrix:\n" << A << endl << endl;  // print the matrix
	cout << "Initial Solution:\n" << X << endl << endl;  // print the initial solution

	MatrixDenoising *denoiseFunction = new MatrixDenoising(A); // create our first order gradient problem
	StiefelParameterization *stiefelParameterization = new StiefelParameterization(N, K);
	GradientProblem stiefelProblem(denoiseFunction, stiefelParameterization);

	GradientProblemSolver::Summary summary = solveGradientProblem(stiefelProblem, X);

	cout << summary.FullReport() << endl;

	// check if X is on the manifold
	cout << "Final Solution:\n" << X << endl << endl;  // print the final solution
	cout << "Is X on Manifold: " << Stiefel_check(X) << endl << endl;
		
	// print the closed form solution
	cout << "Solution by projection (closed form solution should be close to Final Solution):\n" << Stiefel_projection_SVD(A) << endl << endl;

	return 0;
}

