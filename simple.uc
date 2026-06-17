main(.
  Print("Hello").
  n == 42.
  Print(n).
  result == run.identity(n.).
  Print("got:").
  Print(result).
).

identity(x.
  response(x).
).

run.main(.).
