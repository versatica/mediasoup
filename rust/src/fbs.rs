//! Traits for converting between Rust and FlatBuffers data structures.

pub(crate) trait TryFromFbs<'a>: Sized {
    type FbsType;
    type Error;

    fn try_from_fbs(fbs: Self::FbsType) -> Result<Self, Self::Error>;
}

pub(crate) trait FromFbs: Sized {
    type FbsType;

    fn from_fbs(fbs: &Self::FbsType) -> Self;
}

impl<'a, T> TryFromFbs<'a> for Vec<T>
where
    T: TryFromFbs<'a>,
{
    type FbsType = Vec<T::FbsType>;
    type Error = T::Error;

    fn try_from_fbs(fbs: Self::FbsType) -> Result<Self, Self::Error> {
        fbs.into_iter().map(T::try_from_fbs).collect()
    }
}

pub(crate) trait ToFbs: Sized {
    type FbsType;

    fn to_fbs(&self) -> Self::FbsType;
}

impl<T> FromFbs for Option<T>
where
    T: FromFbs,
{
    type FbsType = Option<T::FbsType>;

    fn from_fbs(value: &Self::FbsType) -> Self {
        value.as_ref().map(T::from_fbs)
    }
}

impl<T> FromFbs for Vec<T>
where
    T: FromFbs,
{
    type FbsType = Vec<T::FbsType>;

    fn from_fbs(fbs: &Self::FbsType) -> Self {
        fbs.iter().map(T::from_fbs).collect()
    }
}

impl<T> ToFbs for Vec<T>
where
    T: ToFbs,
{
    type FbsType = Vec<T::FbsType>;

    fn to_fbs(&self) -> Self::FbsType {
        self.iter().map(|item| item.to_fbs()).collect()
    }
}

impl<T> ToFbs for Option<T>
where
    T: ToFbs,
{
    type FbsType = Option<T::FbsType>;

    fn to_fbs(&self) -> Self::FbsType {
        self.as_ref().map(T::to_fbs)
    }
}
