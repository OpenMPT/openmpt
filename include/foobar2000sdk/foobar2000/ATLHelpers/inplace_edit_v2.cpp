#include "stdafx.h"

namespace InPlaceEdit {

	t_size CTableEditHelperV2::ColumnToPosition(t_size col) const {
		PFC_ASSERT( TableEdit_IsColumnEditable(col) );
		pfc::array_t<t_size> colOrder; GrabColumnOrder(colOrder);
		t_size skipped = 0;
		for(t_size walk = 0; walk < colOrder.get_size(); ++walk) {
			const t_size curCol = colOrder[walk];
			if (TableEdit_IsColumnEditable(curCol)) {
				if (curCol == col) return skipped;
				++skipped;
			}
		}
		PFC_ASSERT( !"Should not get here." );
		return ~0;
	}
	t_size CTableEditHelperV2::PositionToColumn(t_size pos) const {
		pfc::array_t<t_size> colOrder; GrabColumnOrder(colOrder);
		t_size skipped = 0;
		for(t_size walk = 0; walk < colOrder.get_size(); ++walk) {
			const t_size curCol = colOrder[walk];
			if (TableEdit_IsColumnEditable(curCol)) {
				if (skipped == pos) return curCol;
				++skipped;
			}
		}
		PFC_ASSERT( !"Should not get here." );
		return ~0;
	}
	t_size CTableEditHelperV2::EditableColumnCount() const {
		const t_size total = TableEdit_GetColumnCount();
		t_size found = 0;
		for(t_size walk = 0; walk < total; ++walk) {
			if (TableEdit_IsColumnEditable(walk)) found++;
		}
		return found;
	}

	bool CTableEditHelperV2::TableEdit_Advance(t_size & item, t_size & subItem, t_uint32 whathappened) {
		//moo
		unsigned _item((unsigned)item), _subItem((unsigned)ColumnToPosition(subItem));
		if (!InPlaceEdit::TableEditAdvance(_item,_subItem,(unsigned) TableEdit_GetItemCount(), (unsigned) EditableColumnCount(), whathappened)) return false;
		item = _item; subItem = PositionToColumn(_subItem);
		return true;
	}
	
	void CTableEditHelperV2::TableEdit_Abort(bool forwardContent) {
		if (this->have_task(KTaskID)) {
			this->orphan_task(KTaskID);
			if (forwardContent && (m_editFlags & KFlagReadOnly) == 0) {
				if (m_editData.is_valid()) {
					pfc::string8 temp(*m_editData);
					TableEdit_SetField(m_editItem,m_editSubItem, temp);
				}
			}
			m_editData.release();
			SetFocus(NULL);
			TableEdit_Finished();
		}

	}
	
	void CTableEditHelperV2::TableEdit_Start(t_size item, t_size subItem) {
		PFC_ASSERT( TableEdit_IsColumnEditable( subItem ) );
		m_editItem = item; m_editSubItem = subItem;
		_ReStart();
	}
	
	void CTableEditHelperV2::_ReStart() {
		PFC_ASSERT( m_editItem < TableEdit_GetItemCount() );
		PFC_ASSERT( m_editSubItem < TableEdit_GetColumnCount() );

		TableEdit_SetItemFocus(m_editItem,m_editSubItem);
		m_editData.new_t();
		t_size lineCount = 1;
		TableEdit_GetField(m_editItem, m_editSubItem, *m_editData, lineCount);
		m_editFlags = TableEdit_GetEditFlags(m_editItem, m_editSubItem);

		RECT rc = TableEdit_GetItemRect(m_editItem, m_editSubItem);
		if (lineCount > 1) {
			rc.bottom = rc.top + (rc.bottom - rc.top) * lineCount;
			m_editFlags |= KFlagMultiLine;
		}
		pfc::com_ptr_t<IUnknown> acl;
		if (!TableEdit_GetAutoComplete(m_editItem, m_editSubItem, acl)) acl.release();

		InPlaceEdit::StartEx(TableEdit_GetParentWnd(), rc, m_editFlags, m_editData, create_task(KTaskID), acl.get_ptr(), ACO_AUTOSUGGEST);
	}

	void CTableEditHelperV2::on_task_completion(unsigned id, unsigned status) {
		if (id == KTaskID) {
			orphan_task(KTaskID);
			if (m_editData.is_valid()) {
				if (status & InPlaceEdit::KEditFlagContentChanged) {
					TableEdit_SetField(m_editItem,m_editSubItem,*m_editData);
				}
				m_editData.release();
			}
			
			if (TableEdit_Advance(m_editItem,m_editSubItem,status)) {
				_ReStart();
			} else {
				TableEdit_Finished();
			}
		}
	}





	void CTableEditHelperV2_ListView::TableEdit_GetColumnOrder(t_size * out, t_size count) const {
		pfc::array_t<int> temp; temp.set_size(count);
		WIN32_OP_D( ListView_GetColumnOrderArray( TableEdit_GetParentWnd(), count, temp.get_ptr() ) );
		for(t_size walk = 0; walk < count; ++walk) out[walk] = temp[walk];
	}

	RECT CTableEditHelperV2_ListView::TableEdit_GetItemRect(t_size item, t_size subItem) const {
		RECT rc;
		WIN32_OP_D( ListView_GetSubItemRect(TableEdit_GetParentWnd(),item,subItem,LVIR_LABEL,&rc) );
		return rc;
	}

	void CTableEditHelperV2_ListView::TableEdit_GetField(t_size item, t_size subItem, pfc::string_base & out, t_size & lineCount) {
		listview_helper::get_item_text( TableEdit_GetParentWnd(), item, subItem, out);
		lineCount = pfc::is_multiline(out) ? 5 : 1;
	}
	void CTableEditHelperV2_ListView::TableEdit_SetField(t_size item, t_size subItem, const char * value) {
		WIN32_OP_D( listview_helper::set_item_text( TableEdit_GetParentWnd(), item, subItem, value) );
	}
	t_size CTableEditHelperV2_ListView::TableEdit_GetItemCount() const {
		LRESULT temp;
		WIN32_OP_D( ( temp = ListView_GetItemCount( TableEdit_GetParentWnd() ) ) >= 0 );
		return (t_size) temp;
	}
	void CTableEditHelperV2_ListView::TableEdit_SetItemFocus(t_size item, t_size subItem) {
		WIN32_OP_D( listview_helper::select_single_item( TableEdit_GetParentWnd(), item ) );
	}
}
