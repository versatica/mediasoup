use std::collections::HashMap;
use std::mem;
use std::sync::{Arc, Mutex};

/// Handler ID keeps event handler in place, once dropped handler will be removed automatically.
///
/// [`HandlerId::detach()`] can be used if it is not desirable for handler to be removed
/// automatically.
#[must_use = "Handler will be unregistered immediately if not used"]
pub struct HandlerId {
    callback: Option<Box<dyn FnOnce() + Send>>,
}

impl HandlerId {
    /// Consumes `HandlerId` and prevents handler from being removed automatically.
    pub fn detach(mut self) {
        // Remove callback such that it is not called in drop implementation
        self.callback.take();
    }
}

impl Drop for HandlerId {
    fn drop(&mut self) {
        if let Some(callback) = self.callback.take() {
            callback();
        }
    }
}

struct Inner<F: ?Sized + Send> {
    handlers: HashMap<usize, Box<F>>,
    next_index: usize,
}

#[derive(Clone)]
pub(crate) struct Bag<F: ?Sized + Send + 'static> {
    inner: Arc<Mutex<Inner<F>>>,
}

impl<F: ?Sized + Send> Default for Bag<F> {
    fn default() -> Self {
        Self {
            inner: Arc::new(Mutex::new(Inner {
                handlers: HashMap::new(),
                next_index: 0,
            })),
        }
    }
}

impl<F: ?Sized + Send> Bag<F> {
    pub(crate) fn add(&self, callback: Box<F>) -> HandlerId {
        let index;

        {
            let mut inner = self.inner.lock().unwrap();

            index = inner.next_index;
            inner.next_index += 1;

            inner.handlers.insert(index, callback);
        }

        let weak_inner = Arc::downgrade(&self.inner);
        HandlerId {
            callback: Some(Box::new(move || {
                if let Some(inner) = weak_inner.upgrade() {
                    inner.lock().unwrap().handlers.remove(&index);
                }
            })),
        }
    }

    /// Call applicator with each handler and keep handlers in the bag
    pub(crate) fn call<A>(&self, applicator: A)
    where
        A: Fn(&Box<F>),
    {
        for callback in self.inner.lock().unwrap().handlers.values() {
            applicator(callback);
        }
    }

    /// Call applicator with each handler and remove handlers from the bag
    pub(crate) fn call_once<A>(&self, applicator: A)
    where
        A: Fn(Box<F>),
    {
        for (_, callback) in mem::take(&mut self.inner.lock().unwrap().handlers).into_iter() {
            applicator(callback);
        }
    }
}

impl<F: Fn() + ?Sized + Send> Bag<F> {
    /// Call each handler without arguments and keep handlers in the bag
    pub(crate) fn call_simple(&self) {
        for callback in self.inner.lock().unwrap().handlers.values() {
            callback();
        }
    }
}

impl<F: FnOnce() + ?Sized + Send> Bag<F> {
    /// Call each handler without arguments and remove handlers from the bag
    pub(crate) fn call_once_simple(&self) {
        for (_, callback) in mem::take(&mut self.inner.lock().unwrap().handlers).into_iter() {
            callback();
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::sync::atomic::{AtomicUsize, Ordering};

    #[test]
    fn fn_once() {
        let bag = Bag::<dyn FnOnce() + Send>::default();
        let calls = Arc::new(AtomicUsize::new(0));

        {
            let calls = Arc::clone(&calls);
            bag.add(Box::new(move || {
                calls.fetch_add(1, Ordering::SeqCst);
            }))
            .detach();
        }
        {
            let calls = Arc::clone(&calls);
            drop(bag.add(Box::new(move || {
                calls.fetch_add(1, Ordering::SeqCst);
            })));
        }
        bag.call_once(|callback| {
            callback();
        });

        assert_eq!(calls.load(Ordering::SeqCst), 1);

        {
            let calls = Arc::clone(&calls);
            bag.add(Box::new(move || {
                calls.fetch_add(1, Ordering::SeqCst);
            }))
            .detach();
        }
        bag.call_once_simple();

        assert_eq!(calls.load(Ordering::SeqCst), 2);
    }

    #[test]
    fn fn_regular() {
        let bag = Bag::<dyn Fn() + Send>::default();
        let calls = Arc::new(AtomicUsize::new(0));

        {
            let calls = Arc::clone(&calls);
            bag.add(Box::new(move || {
                calls.fetch_add(1, Ordering::SeqCst);
            }))
            .detach();
        }
        {
            let calls = Arc::clone(&calls);
            drop(bag.add(Box::new(move || {
                calls.fetch_add(1, Ordering::SeqCst);
            })));
        }
        bag.call(|callback| {
            callback();
        });

        assert_eq!(calls.load(Ordering::SeqCst), 1);

        bag.call_simple();

        assert_eq!(calls.load(Ordering::SeqCst), 2);
    }
}
