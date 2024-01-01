kal_uint32 set_brack_offset(kal_uint32_offset)
{
#if defined(CONDITION)
  kal_uint32 current_thread = THIS_THREAD_TYPE();
  kal_uint32 current_layer = get_current_layer();
  return VISIT_TREAD_ADDRESS(current_thread, current_layer);
#endif
}
