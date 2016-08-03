define plist
  set $ptr = $arg0
  set $cnt = 0
  printf "LinkedList\n"
  while $ptr != 0
    set $cnt += 1
    printf "list_ = 0x%x\n", $ptr
    set $ptr = *((void **)$ptr)
  end
  printf "length_ = %d\n", $cnt
end

define pfree_list
  set $this = $arg0
  set $list_ = (void **)$this
  set $numbers = (uint32_t *)($this + sizeof(void *))
  set $length_ = (uint32_t *)$numbers++
  set $lowater_ = (uint32_t *)$numbers++
  set $max_length_ = (uint32_t *)$numbers++
  set $length_overages_ = (uint32_t *)$numbers++
  printf "FreeList(0x%x)\n", $this
  printf "list_(0x%x) = 0x%x\n", $list_, *$list_
  printf "length_(0x%x) = %d\n", $length_, *$length_
  printf "lowater_(0x%x) = %d\n", $lowater_, *$lowater_
  printf "max_length_(0x%x) = %d\n", $max_length_, *$max_length_
  printf "length_overages_(0x%x) = %d\n", $length_overages_, *$length_overages_
end

define pthread_cache
  set $kNumClasses = 86

  set $this = $arg0
  set $ptr = (void **)$this
  set $next_ = $ptr++
  set $prev_ = $ptr++
  set $ptr = (size_t *)$ptr
  set $size_ = $ptr++
  set $max_size_ = $ptr++
  set $bytes_until_sample_ = $ptr++
  set $rnd_ = (uint64_t *)($ptr++)
  set $list_ = (void *)$ptr
  set $ptr = (void *)$ptr
  set $ptr += $kNumClasses * (3 * 8)
  set $tid_ = (pthread_t *)$ptr++
  set $in_setspecific_ = (bool *)$ptr

  printf "ThreadCache\n"
  printf "next_(0x%x) = 0x%x\n", $next_, *$next_
  printf "prev_(0x%x) = 0x%x\n", $prev_, *$prev_
  printf "size_(0x%x) = %lu\n", $size_, *$size_
  printf "max_size_(0x%x) = %lu\n", $max_size_, *$max_size_
  printf "Sampler.bytes_until_sample_(0x%x) = %lu\n", $bytes_until_sample_, *$bytes_until_sample_
  printf "Sampler.rnd_(0x%x) = %lu\n", $rnd_, *$rnd_
  printf "list_(0x%x) = \n", $list_

  set $i = 0
  set $l = $list_
  while $i < $kNumClasses
    pfree_list $l
    printf "\n"
    set $l += 0x18
    set $i++
  end
end
