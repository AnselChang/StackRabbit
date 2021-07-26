#include "../include/eval.h"
#include "../include/move_result.h"
#include "../include/utils.h"

#include <math.h>
#include <vector>
// #include "data/ranksOutput.cc"
using namespace std;

const int USE_RANKS = 0;

/**
 * A crude way to evaluate a surface for when I'm debugging and don't want to load the surfaces every time I
 * run.
 */
float calculateFlatness(int surfaceArray[10]) {
  float score = 30;
  for (int i = 0; i < 8; i++) {
    int diff = surfaceArray[i + 1] - surfaceArray[i];
    if (diff != 0) {
      // printf("Diff of %d => %f, so far %f\n", abs(diff), pow(abs(diff), 2.5), score);
      score -= pow(abs(diff), 2.5);
      // printf("%f\n", score);
    }
  }
  return score;
}

/** Gets the value of a surface. */
float rateSurface(int surfaceArray[10]) {
  // Backup option in case ranks aren't loaded
  if (!USE_RANKS) {
    return calculateFlatness(surfaceArray);
  }
  // Convert the surface array into the custom base-9 encoding
  int index = 0;
  for (int i = 0; i < 8; i++) {
    int diff = surfaceArray[i + 1] - surfaceArray[i];
    index *= 9;
    index += diff + 3;
  }
  // printf("index: %d\n", index);
  return 1.0; // So it compiles without the ranks
  // return surfaceRanksRaw[index] * 0.1;
}

float getAverageHeightFactor(int surfaceArray[10], int wellColumn, FastEvalWeights weights) {
  float totalHeight = 0;
  float weight = wellColumn >= 0 ? 0.1 : 0.111111;
  for (int i = 0; i < 10; i++) {
    if (i == wellColumn) {
      continue;
    }
    totalHeight += surfaceArray[i] * weight;
  }
  return weights.avgHeightCoef * totalHeight;
}

float getCoveredWellBurnFactor(int board[20], int wellColumn, FastEvalWeights weights) {
  if (wellColumn == -1) {
    return 0;
  }
  int mask = (1 << (9 - wellColumn));
  int wellCells = 0;
  for (int r = 0; r < 20; r++) {
    if (board[r] & 1) {
      wellCells++;
    }
  }
  return weights.burnCoef * wellCells;
}

float fastEval(GameState gameState,
               SimState lockPlacement,
               EvalContext evalContext,
               FastEvalWeights weights) {
  // Get the game state to evaluate
  GameState newState = {{}, {}, gameState.adjustedNumHoles, gameState.lines};
  int numLinesCleared = getNewBoardAndLinesCleared(gameState.board, lockPlacement, newState.board);  
  int numNewHoles =
      getNewSurfaceAndNumNewHoles(gameState.surfaceArray, lockPlacement, evalContext, newState.surfaceArray);
  if (numLinesCleared > 0) {
    newState.adjustedNumHoles =
        updateSurfaceAndHolesAfterLineClears(newState.surfaceArray, newState.board, numLinesCleared);
  } else {
    newState.adjustedNumHoles += numNewHoles;
  }

  // Calculate all the factors
  float surfaceFactor = weights.surfaceCoef * rateSurface(newState.surfaceArray);
  float avgHeightFactor = getAverageHeightFactor(newState.surfaceArray, evalContext.wellColumn, weights);
  float lineClearFactor = numLinesCleared == 4 ? weights.tetrisCoef : weights.burnCoef * numLinesCleared;
  float holeFactor = weights.holeCoef * newState.adjustedNumHoles;
  float coveredWellBurnFactor = getCoveredWellBurnFactor(newState.board, evalContext.wellColumn, weights);

  float total = surfaceFactor + avgHeightFactor + lineClearFactor + holeFactor + coveredWellBurnFactor;

  // Logging
  if (LOGGING_ENABLED) {
    maybePrint("\nEvaluating possibility %d %d %d\n",
               lockPlacement.rotationIndex,
               lockPlacement.x - SPAWN_X,
               lockPlacement.y);
    printBoard(newState.board);
    for (int i = 0; i < 9; i++) {
      maybePrint("%d ", newState.surfaceArray[i]);
    }
    maybePrint("%d\n", newState.surfaceArray[9]);
    maybePrint(
        "Surface %01f, AvgHeight %01f, LineClear %01f, Hole %01f, CoveredWellBurn %01f\t Total: %01f\n",
        surfaceFactor,
        avgHeightFactor,
        lineClearFactor,
        holeFactor,
        coveredWellBurnFactor,
        total);
  }

  return total;
}
