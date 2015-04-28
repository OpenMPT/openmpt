namespace CF {
	template<typename obj_t, typename arg_t> class _inMainThread : public main_thread_callback {
	public:
		_inMainThread(obj_t const & obj, const arg_t & arg) : m_obj(obj), m_arg(arg) {}
		
		void callback_run() {
			if (m_obj.IsValid()) callInMainThread::callThis(&*m_obj, m_arg);
		}
	private:
		obj_t m_obj;
		arg_t m_arg;
	};

	template<typename TWhat> class CallForwarder {
	public:
		CallForwarder(TWhat * ptr) { m_ptr.new_t(ptr); }

		void Orphan() {*m_ptr = NULL;}
		bool IsValid() const { return *m_ptr != NULL; }
		bool IsEmpty() const { return !IsValid(); }

		TWhat * operator->() const {
			PFC_ASSERT( IsValid() );
			return *m_ptr;
		}

		TWhat & operator*() const {
			PFC_ASSERT( IsValid() );
			return **m_ptr;
		}

		template<typename arg_t>
		void callInMainThread(const arg_t & arg) {
			main_thread_callback_add( new service_impl_t<_inMainThread< CallForwarder<TWhat>, arg_t> >(*this, arg) );
		}
	private:
		pfc::rcptr_t<TWhat*> m_ptr;
	};

	template<typename TWhat> class CallForwarderMaster : public CallForwarder<TWhat> {
	public:
		CallForwarderMaster(TWhat * ptr) : CallForwarder<TWhat>(ptr) {}
		~CallForwarderMaster() { this->Orphan(); }

		PFC_CLASS_NOT_COPYABLE(CallForwarderMaster, CallForwarderMaster<TWhat>);
	};

}