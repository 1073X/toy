#pragma once

#include <limits>
#include <array>

namespace toy {
    namespace reference {

        template<typename T> class container {
            using item_type = T;
            using id_type = typename item_type::id_type;
            using id_limits = std::numeric_limits<id_type>;

            // [bucket(1/2)][slot(1/4)][item(1/4)]

            static id_type const bucket_id_digits = id_limits::digits / 2;
            static id_type const slot_id_digits = id_limits::digits / 4;
            static id_type const item_id_digits = id_limits::digits / 4;

            static id_type const buckets_capacity = id_limits::max() >> (id_limits::digits - bucket_id_digits);
            static id_type const bucket_capacity = id_limits::max() >> (id_limits::digits - slot_id_digits);
            static id_type const slot_capacity = id_limits::max() >> (id_limits::digits - item_id_digits);

            using slot_type = std::array<item_type, slot_capacity>;
            using bucket_type = std::array<slot_type*, bucket_capacity>;
            using buckets_type = std::array<bucket_type*, buckets_capacity>;

            union id_wrapper {
                id_type id;
                struct {
                    id_type item_id : item_id_digits;
                    id_type slot_id : slot_id_digits;
                    id_type bucket_id : bucket_id_digits;
                };
            };

            public:
            ~container() {
                for(auto pbucket : _buckets) {
                    if(pbucket) {
                        for(auto pslot : *pbucket) {
                            if(pslot) {
                                delete pslot;
                            }
                        }
                    }
                }
            }

            template<typename ... ARGS> auto create(id_type id, ARGS ... args) {
                auto pslot = retrieve_slot(id);
                if(!pslot) {
                    return (item_type*)nullptr;
                }

                id_wrapper wrapper { id };
                auto* pitem = &pslot->at(wrapper.item_id);
                if(pitem->id != item_type::invalid_id) {
                    return (item_type*)nullptr;
                }

                return new(pitem) item_type(id, args ...);
            }

            template<typename ... ARGS> auto retrieve(id_type id, ARGS ... args) {
                auto pslot = retrieve_slot(id);
                if(!pslot) {
                    return (item_type*)nullptr;
                }

                id_wrapper wrapper { id };
                auto* pitem = &pslot->at(wrapper.item_id);
                if(pitem->id == item_type::invalid_id) {
                    new(pitem) item_type(id, args ...);
                }

                return pitem;
            }

            auto remove(id_type id) {
                auto pitem = find(id);
                if(!pitem) {
                    return;
                }

                pitem->~item_type();
            }

            auto find(id_type id) {
                id_wrapper wrapper { id };

                auto pbucket = _buckets[wrapper.bucket_id];
                if(!pbucket) {
                    return (item_type*)nullptr;
                }

                auto pslot = pbucket->at(wrapper.slot_id);
                if(!pslot) {
                    return (item_type*)nullptr;
                }

                auto pitem = &pslot->at(wrapper.item_id);
                if(item_type::invalid_id == pitem->id) {
                    return (item_type*)nullptr;
                }

                return pitem;
            }

            private:
            auto retrieve_slot(id_type id) -> slot_type* {
                if(id == item_type::invalid_id) {
                    return (slot_type*)nullptr;
                }

                id_wrapper wrapper { id };

                if(!_buckets[wrapper.bucket_id]) {
                    _buckets[wrapper.bucket_id] = new bucket_type { nullptr };
                }
                auto& bucket = *_buckets[wrapper.bucket_id];

                if(!bucket[wrapper.slot_id]) {
                    bucket[wrapper.slot_id] = new slot_type;
                }

                return bucket[wrapper.slot_id];
            }

            private:
            buckets_type _buckets { nullptr };
        };
    }
}
