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

pub(crate) trait ToFbs: Sized {
    type FbsType;

    fn to_fbs(&self) -> Self::FbsType;
}

pub(crate) trait IntoFbs: Sized {
    type FbsType;

    fn into_fbs(self) -> Self::FbsType;
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

impl<T> ToFbs for Option<T>
where
    T: ToFbs,
{
    type FbsType = Option<T::FbsType>;

    fn to_fbs(&self) -> Self::FbsType {
        self.as_ref().map(T::to_fbs)
    }
}
