
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */
#include <string.h>

#include "../include/piece_ranges.h"
#include "../include/tetrominoes.h"
#include "../include/params.h"
#include "../include/eval_context.h"
// I have to include the C++ files here due to a complication of node-gyp. Consider this the equivalent
// of listing all the C++ sources in the makefile (Node-gyp seems to only work with 1 source rn).
#include "eval.cc"
#include "move_result.cc"
#include "move_search.cc"
#include "playout.cc"
#include "high_level_search.cc"
// #include "../data/ranksOutput.cc"

#define USE_RANDOM_SEQUENCE false

std::string mainProcess(char const *inputStr) {
  maybePrint("Input string %s\n", inputStr);

  // Init empty data structures
  GameState startingGameState = {
    /* board= */ {},
    /* surfaceArray= */ {},
    /* adjustedNumHole= */ 0,
    /* lines= */ 0,
    /* level= */ 0
  };
  Piece curPiece;
  Piece nextPiece;
  std::string inputFrameTimeline;

  // Loop through the other args
  std::string s = std::string(inputStr + 201); // 201 = the length of the board string + 1 for the delimiter
  std::string delim = "|";
  auto start = 0U;
  auto end = s.find(delim);
  for (int i = 0; end != std::string::npos; i++) {
    std::string arg = s.substr(start, end - start);
    int argAsInt = atoi(arg.c_str());
    maybePrint("ARG %d: %d\n", i, argAsInt);
    switch (i) {
    case 0:
      startingGameState.level = argAsInt;
    case 1:
      startingGameState.lines = argAsInt;
      break;
    case 2:
      curPiece = PIECE_LIST[argAsInt];
      break;
    case 3:
      nextPiece = PIECE_LIST[argAsInt];
      break;
    case 4:
      inputFrameTimeline = arg;
    default:
      break;
    }

    start = (int) end + (int) delim.length();
    end = s.find(delim, start);
  }
  int wellColumn = isLineout(startingGameState) ? -1 : 9;
  // Fill in the data structures
  encodeBoard(inputStr, startingGameState.board);
  getSurfaceArray(startingGameState.board, startingGameState.surfaceArray);
  startingGameState.adjustedNumHoles = updateSurfaceAndHolesAfterLineClears(startingGameState.surfaceArray, startingGameState.board, wellColumn);
  
  // Calculate global context for the 3 possible gravity values
  const PieceRangeContext pieceRangeContextLookup[3] = {
    getPieceRangeContext(inputFrameTimeline.c_str(), 1),
    getPieceRangeContext(inputFrameTimeline.c_str(), 2),
    getPieceRangeContext(inputFrameTimeline.c_str(), 3),
  };
  const EvalContext context = getEvalContext(startingGameState, pieceRangeContextLookup);

  if (LOGGING_ENABLED) {
    printBoard(startingGameState.board);
    printBoardBits(startingGameState.board);
  }

  std::string lookupMapEncoded = getLockValueLookupEncoded(startingGameState, curPiece, nextPiece, DEPTH_2_PRUNING_BREADTH, &context, pieceRangeContextLookup);
  return lookupMapEncoded;
}
