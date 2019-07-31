CREATE OR REPLACE FUNCTION public.pgencode7(IN by_data bytea, OUT t_encode text)
RETURNS SETOF text
AS '$libdir/pgcodec7', 'pgencode7'  LANGUAGE C IMMUTABLE;
GRANT EXECUTE ON FUNCTION public.pgencode7(bytea) TO public;

CREATE OR REPLACE FUNCTION public.pgdecode7(IN t_encodes text[])
RETURNS bytea
AS '$libdir/pgcodec7', 'pgdecode7'  LANGUAGE C IMMUTABLE;
GRANT EXECUTE ON FUNCTION public.pgdecode7(text[]) TO public;