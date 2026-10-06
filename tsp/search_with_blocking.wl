ClearAll["Global`*"];

prepareMatrix[m_] := 
 Module[{a = m, n = Length[m]}, Do[a[[i, i]] = Infinity, {i, n}];
  a]

routeCost[route_, m_] := 
 If[Length[route] < 2, Infinity, 
  Total[m[[#1, #2]] & @@@ Partition[route, 2, 1]]]

randomRoute[n_] := Join[{1}, RandomSample[Range[2, n]], {1}]

routeKey[route_] := Drop[route, -1]

tabuQ[a_, b_, tabuList_, it_] := 
 AnyTrue[tabuList, (#[[1]] == a && #[[2]] == b && #[[3]] >= it) &]

cleanTabu[tabuList_, it_] := Select[tabuList, #[[3]] >= it &]

reader[filename_String] := 
 Module[{data, n, matrix}, data = Import[filename, "Table"];
  n = data[[1, 1]];
  matrix = data[[2 ;; n + 1]];
  If[Length[matrix] != n || AnyTrue[matrix, Length[#] != n &], 
   Print["Ошибка: неверный формат матрицы в файле ", filename]; Abort[]];
  prepareMatrix[matrix]]

tabuOneRun[m_, start_, tenure_, maxIt_, limit_] := 
 Module[{n = Length[m], cur, curCost, best, bestCost, tabuList = {}, 
   noImp = 0, iters = 0, foundNeigh, bestCand, bestCandCost, 
   removedEdges, addedEdges, pi, ci, ni, pj, cj, nj, oldE, newE, oldC,
    newC, c, cand, key}, cur = start; curCost = routeCost[cur, m];
  best = cur; bestCost = curCost;
  Do[foundNeigh = False; bestCandCost = Infinity; bestCand = {};
   removedEdges = {}; addedEdges = {};
   Do[pi = cur[[i - 1]]; ci = cur[[i]]; ni = cur[[i + 1]];
    pj = cur[[j - 1]]; cj = cur[[j]]; nj = cur[[j + 1]];
    If[j == i + 1, oldE = {{pi, ci}, {ci, cj}, {cj, nj}};
     newE = {{pi, cj}, {cj, ci}, {ci, nj}};
     oldC = m[[pi, ci]] + m[[ci, cj]] + m[[cj, nj]];
     newC = m[[pi, cj]] + m[[cj, ci]] + m[[ci, nj]],
     
     oldE = {{pi, ci}, {ci, ni}, {pj, cj}, {cj, nj}};
     newE = {{pi, cj}, {cj, ni}, {pj, ci}, {ci, nj}};
     oldC = m[[pi, ci]] + m[[ci, ni]] + m[[pj, cj]] + m[[cj, nj]];
     newC = m[[pi, cj]] + m[[cj, ni]] + m[[pj, ci]] + m[[ci, nj]]];
    If[oldC === Infinity || newC === Infinity, Continue[]];
    c = curCost + (newC - oldC);
    If[AnyTrue[newE, tabuQ[#[[1]], #[[2]], tabuList, it] &] && 
      c >= bestCost, Continue[]];
    cand = cur; cand[[{i, j}]] = cand[[{j, i}]];
    key = routeKey[cand];
    If[! foundNeigh || c < bestCandCost, foundNeigh = True;
     bestCandCost = c; bestCand = cand;
     removedEdges = oldE;   (*для добавления в табу*)], {i, 2, 
     n - 1}, {j, i + 1, n}];
   If[! foundNeigh, Break[]];
   cur = bestCand; curCost = bestCandCost;
   tabuList = 
    Join[tabuList, ({#[[1]], #[[2]], it + tenure} & /@ removedEdges)];
   tabuList = cleanTabu[tabuList, it];
   iters++;
   If[curCost < bestCost, best = cur; bestCost = curCost; noImp = 0, 
    noImp++];
   If[noImp >= limit, Break[]], {it, 1, maxIt}];
  <|"route" -> best, "cost" -> bestCost, "iterations" -> iters|>]

Options[tabuSearch] = {"Tenure" -> Automatic, 
   "MaxIterations" -> Automatic, "NoImprovementLimit" -> Automatic, 
   "Restarts" -> Automatic, "Verbose" -> True};

tabuSearch[m_, OptionsPattern[]] := 
 Module[{n = Length[m], tenure, maxIt, limit, restarts, best, 
   bestCost = Infinity, totalIt = 0, r, t0, sol}, 
  tenure = 
   Replace[OptionValue["Tenure"], 
    Automatic :> Max[5, Min[30, Round[0.5 n]]]];
  maxIt = Replace[OptionValue["MaxIterations"], Automatic :> 100 n];
  limit = 
   Replace[OptionValue["NoImprovementLimit"], Automatic :> 10 n];
  restarts = 
   Replace[OptionValue["Restarts"], Automatic :> Max[1, Floor[n/5]]];
  SeedRandom[];
  If[OptionValue["Verbose"], 
   Print["Параметры: tenure=", tenure, ", итераций=", maxIt, ", лимит=",
     limit, ", рестартов=", restarts]];
  t0 = AbsoluteTime[];
  Do[sol = tabuOneRun[m, randomRoute[n], tenure, maxIt, limit];
   totalIt += sol["iterations"];
   If[OptionValue["Verbose"], 
    Print["  запуск ", r, "/", restarts, ": стоимость = ", sol["cost"],
      If[sol["cost"] < bestCost, "  (НОВЫЙ ЛУЧШИЙ!)", ""]]];
   If[sol["cost"] < bestCost, bestCost = sol["cost"];
    best = sol["route"]], {r, restarts}];
  <|"route" -> best, "cost" -> bestCost, "iterations" -> totalIt, 
   "restarts" -> restarts, "time" -> AbsoluteTime[] - t0, 
   "tenure" -> tenure|>]

printResult[res_, n_] := Module[{}, Print["\n=== ОТВЕТ ==="];
  Print["Маршрут: ", Row[res["route"], " -> "]];
  Print["Стоимость: ", res["cost"]];
  Print["Итераций: ", res["iterations"], " (всего запусков: ", 
   res["restarts"], ")"];
  Print["Время: ", NumberForm[res["time"], {10, 6}], " сек."];
  Print["Параметр tenure: ", res["tenure"]];]

runFromFile[filename_String, opts : OptionsPattern[]] := 
 Module[{m, res}, Print["Чтение матрицы из файла: ", filename];
  m = reader[filename];
  Print["Матрица ", Length[m], "x", Length[m], " загружена."];
  res = tabuSearch[m, opts];
  printResult[res, Length[m]];
  res]

runFromFile["30_points.txt"]