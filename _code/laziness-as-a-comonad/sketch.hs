import Control.Applicative


------------ The Comonad type class ------------
class Functor w => Comonad w where
  extract :: w a -> a

  duplicate :: w a -> w (w a)
  duplicate = extend id

  extend :: (w a -> b) -> w a -> w b
  extend f = fmap f . duplicate


------------ Lazy ------------
data Lazy a = Lazy (() -> a)

eval :: Lazy a -> a
eval (Lazy lx) = lx ()

instance Functor Lazy where
  -- fmap :: (a -> b) -> Lazy a -> Lazy b
  fmap f lx = Lazy (\_ -> f (eval lx))

instance Comonad Lazy where
  -- extract :: Lazy a -> a
  extract = eval

  -- duplicate :: Lazy a -> Lazy (Lazy a)
  duplicate lx = Lazy (\_ -> lx)

  -- extend :: (Lazy a -> b) -> Lazy a -> Lazy b
  extend f lx = Lazy (\_ -> f lx)


------------ The Identity Functor ------------
data Identity a = Identity a deriving Show

instance Functor Identity where
  fmap f (Identity x) = Identity (f x)

instance Applicative Identity where
  pure = Identity
  (Identity f) <*> (Identity x) = Identity (f x)

instance Comonad Identity where
  extract (Identity x) = x
  duplicate (Identity x) = Identity (Identity x)
  extend f (Identity x) = Identity (f (Identity x))


------------ Product ------------
data Product e a = Product e a deriving Show

instance Functor (Product e) where
  fmap f (Product e a) = Product e (f a)

instance Comonad (Product e) where
  extract (Product e a) = a
  duplicate (Product e a) = Product e (Product e a)


------------ Logical (sketch) ------------
class Logical b where
    if_ :: (Comonad w) => w b -> w x -> w x -> w x

    and_ :: (Comonad w) => w b -> w b -> w b
    and_ x y = if_ x y x

    or_ :: (Comonad w) => w b -> w b -> w b
    or_ x y = if_ x x y

    eval_if :: (Comonad w) => w b -> w x -> w x -> x
    eval_if c t e = extract $ if_ c t e

instance Logical Bool where
    if_ c t e = if (extract c) then t else e


main = return 1
