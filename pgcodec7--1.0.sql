CREATE OR REPLACE FUNCTION public.pgencode7(IN i_encodelen integer, IN by_data bytea, OUT t_encode text)
RETURNS SETOF text
AS '$libdir/pgcodec7', 'pgencode7'  LANGUAGE C IMMUTABLE;
GRANT EXECUTE ON FUNCTION public.pgencode7(integer, bytea) TO public;

CREATE OR REPLACE FUNCTION public.pgdecode7(IN i_decodelen integer, IN t_encodes text[])
RETURNS bytea
AS '$libdir/pgcodec7', 'pgdecode7'  LANGUAGE C IMMUTABLE;
GRANT EXECUTE ON FUNCTION public.pgdecode7(integer, text[]) TO public;
