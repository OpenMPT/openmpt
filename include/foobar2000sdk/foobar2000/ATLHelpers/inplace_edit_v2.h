namespace InPlaceEdit {
	class NOVTABLE CTableEditHelperV2 : protected completion_notify_receiver {
	protected:	
		virtual RECT TableEdit_GetItemRect(t_size item, t_size subItem) const = 0;
		virtual void TableEdit_GetField(t_size item, t_size subItem, pfc::string_base & out, t_size & lineCount) = 0;
		virtual void TableEdit_SetField(t_size item, t_size subItem, const char * value) = 0;
		virtual HWND TableEdit_GetParentWnd() const = 0;
		virtual bool TableEdit_Advance(t_size & item, t_size & subItem, t_uint32 whathappened);
		virtual void TableEdit_Finished() {}
		virtual t_size TableEdit_GetItemCount() const = 0;
		virtual t_size TableEdit_GetColumnCount() const = 0;
		virtual void TableEdit_SetItemFocus(t_size item, t_size subItem) = 0;
		virtual bool TableEdit_IsColumnEditable(t_size subItem) const {return true;}
		virtual void TableEdit_GetColumnOrder(t_size * out, t_size count) const {order_helper::g_fill(out,count);}
		virtual t_uint32 TableEdit_GetEditFlags(t_size item, t_size subItem) const {return 0;}
		virtual bool TableEdit_GetAutoComplete(t_size item, t_size subItem, pfc::com_ptr_t<IUnknown> & out) {return false;}
		void TableEdit_Start(t_size item, t_size subItem);
		void TableEdit_Abort(bool forwardContent);
		bool TableEdit_IsActive() const {return have_task(KTaskID);}

		void on_task_completion(unsigned p_id,unsigned p_status);
	private:
		t_size ColumnToPosition(t_size col) const;
		t_size PositionToColumn(t_size pos) const;
		t_size EditableColumnCount() const;
		void GrabColumnOrder(pfc::array_t<t_size> & buffer) const {buffer.set_size(TableEdit_GetColumnCount()); TableEdit_GetColumnOrder(buffer.get_ptr(), buffer.get_size());}
		void _ReStart();
		
		t_size m_editItem, m_editSubItem;
		t_uint32 m_editFlags;
		pfc::rcptr_t<pfc::string8> m_editData;
		static const unsigned KTaskID = 0x6f0a3de6;
	};




	class NOVTABLE CTableEditHelperV2_ListView : public CTableEditHelperV2 {
	public:
		RECT TableEdit_GetItemRect(t_size item, t_size subItem) const;
		void TableEdit_GetField(t_size item, t_size subItem, pfc::string_base & out, t_size & lineCount);
		void TableEdit_SetField(t_size item, t_size subItem, const char * value);

		t_size TableEdit_GetColumnCount() const {return (t_size) ListView_GetColumnCount(TableEdit_GetParentWnd());}

		t_size TableEdit_GetItemCount() const;
		void TableEdit_SetItemFocus(t_size item, t_size subItem);

		void TableEdit_GetColumnOrder(t_size * out, t_size count) const;
	};
}
