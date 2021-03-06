#include <Rcpp.h>
using namespace Rcpp;

//' This function will compute the identification risk for a dataset with synthetic continuous and categorical variables.
//' @param origdata dataframe of the origonal data
//' @param syndata list of the different synthetic dataframes
//' @param known vector of the names of the columns in the dataset assumed to be known
//' @param syn vector of the names of the columns in the dataset that are synthetic
//' @param radius radius to compare with for continous variables. Radius is either percentage (default) or fixed.
//' Radius can be the same for all continuous variables or specific to each. To specify for each use a vector, with
//' the radii ordered in the same order those columns appear in the dataset.
//' @param percentage true for a percentage radius, false for a constant radius
//' @param euclideanDist true for a euclidean distance radius, false otherwise
//' @param categoricalVector Boolean vector corresponding to the number of columns in the data, true means that column is categorical.
// [[Rcpp::export(.IdentificationRiskContinuousC)]]
Rcpp::List IdentificationRiskContinuousC(Rcpp::NumericMatrix dataMatrix, int rows, int cols, Rcpp::List syndataMatrices,
                              int num, NumericVector knowncols, int numKnown, NumericVector syncols, int numSyn, 
                              NumericVector radius, int percentage, int euclideanDist, NumericVector categoricalVector) {
  
  Rcpp::NumericMatrix cMatrix(rows, num);
  Rcpp::NumericMatrix tMatrix(rows, num);
  Rcpp::NumericMatrix kMatrix(rows, num);
  Rcpp::NumericMatrix fMatrix(rows, num);
  Rcpp::NumericMatrix riskMatrix(rows, num);
  
  NumericVector sArray(num);
  NumericVector expArray(num);
  NumericVector trueRateArray(num);
  NumericVector falseRateArray(num);
  
  char match_k[rows];
  Rcpp::NumericMatrix syndataMatrix;
  
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < num; j++) {
      SEXP temp = syndataMatrices[j];
      syndataMatrix = Rcpp::NumericMatrix(temp);
      memset(match_k, 1, rows);
      
      for (int k = 0; k < rows; k++) {
        // try to match person i with every person in the synthetic set
        // by first checking if the known variables
        double currRadius = 0;
        if (euclideanDist) {
          for (int l = 0; l < numKnown; l++) {
            if (categoricalVector[knowncols[l]] == 0 && percentage) {
              currRadius += pow(radius[knowncols[l]] * dataMatrix(i, knowncols[l]), 2);
            } else if (categoricalVector[knowncols[l]] == 0) {
              currRadius += pow(radius[knowncols[l]], 2);
            }
          }
          currRadius = pow(currRadius, 0.5);
        }
        for (int l = 0; l < numKnown; l++) {
          if (categoricalVector[knowncols[l]] && dataMatrix(i,knowncols[l]) != syndataMatrix(k, knowncols[l])) {
            match_k[k] = 0;
            break;
          } else if (categoricalVector[knowncols[l]] == 0) {
            if (euclideanDist == 0) {
              currRadius = radius[knowncols[l]] * dataMatrix(i, knowncols[l]);
              if (!percentage) {
                currRadius = radius[knowncols[l]];
              } 
            }
            if (syndataMatrix(k, knowncols[l]) >= dataMatrix(i, knowncols[l]) + currRadius ||
                syndataMatrix(k, knowncols[l]) <= dataMatrix(i, knowncols[l]) - currRadius) {
              match_k[k] = 0;
              break;
            }
          }
        }
        // then if those match check the synthetic ones
        if (euclideanDist) {
          for (int l = 0; l < numSyn; l++) {
            if (categoricalVector[syncols[l]] == 0 && percentage) {
              currRadius += pow(radius[syncols[l]] * dataMatrix(i, syncols[l]), 2);
            } else if (categoricalVector[syncols[l]] == 0) {
              currRadius += pow(radius[syncols[l]], 2);
            }
          }
          currRadius = pow(currRadius, 0.5);
        }
        if (match_k[k]) {
          for (int l = 0; l < numSyn; l++) {
            if (categoricalVector[syncols[l]] && dataMatrix(i,syncols[l]) != syndataMatrix(k,syncols[l])) {
              match_k[k] = 0;
              break;
            } else if (categoricalVector[syncols[l]] == 0) {
              if (euclideanDist == 0) {
                currRadius = radius[syncols[l]] * dataMatrix(i, syncols[l]);
                if (!percentage) {
                  currRadius = radius[syncols[l]];
                }
              }
              if (syndataMatrix(k, syncols[l]) >= dataMatrix(i, syncols[l]) + currRadius || 
                  syndataMatrix(k, syncols[l]) <= dataMatrix(i, syncols[l]) - currRadius) {
                match_k[k] = 0;
                break;
              }
            }
          }
          if (match_k[k]) {
            cMatrix(i,j) = cMatrix(i,j) + 1;
          }
        }
      }
      
      if (cMatrix(i,j) == 1) {
        sArray[j] = sArray[j] + 1;
      }
      
      if (cMatrix(i,j) != 0) {
        tMatrix(i,j) = match_k[i];
        riskMatrix(i,j) = tMatrix(i,j) / cMatrix(i,j);
        expArray[j] += tMatrix(i, j)/cMatrix(i,j);
      } else {
        tMatrix(i,j) = 1;
        riskMatrix(i,j) = 0;
      }
      
      kMatrix(i,j) = (cMatrix(i,j) * tMatrix(i,j)) == 1;
      fMatrix(i,j) = (cMatrix(i,j) * (1 - tMatrix(i,j))) == 1;
      
      trueRateArray[j] = trueRateArray[j] + kMatrix(i,j);
      falseRateArray[j] = falseRateArray[j] + fMatrix(i,j);
    }
  }
  
  for (int i = 0; i < num; i++) {
    trueRateArray[i] = trueRateArray[i] / rows;
    falseRateArray[i] = falseRateArray[i] / sArray[i];
  }
  
  return List::create(Named("s_vector")=sArray,
                      Named("exp.risk_vector")=expArray,
                      Named("true.rate_vector")=trueRateArray,
                      Named("false.rate_vector")=falseRateArray,
                      Named("c_vector")=cMatrix,
                      Named("T_vector")=tMatrix,
                      Named("K_vector")=kMatrix,
                      Named("F_vector")=fMatrix,
                      Named("Risk_vector")=riskMatrix);
}
