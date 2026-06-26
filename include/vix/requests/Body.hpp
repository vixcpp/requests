/**
 *
 *  @file Body.hpp
 *  @author Gaspard Kirira
 *
 *  @brief Request body helpers for the Vix requests module.
 *
 *  Copyright 2026, Gaspard Kirira.
 *  All rights reserved.
 *  https://github.com/vixcpp/requests
 *
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix Requests
 *
 */

#ifndef VIX_REQUESTS_BODY_HPP
#define VIX_REQUESTS_BODY_HPP

#include <vix/requests/Params.hpp>

#include <cstddef>
#include <initializer_list>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace vix::requests
{
  /**
   * @brief Request body type.
   */
  enum class BodyType
  {
    Empty,
    Raw,
    Json,
    Form,
    Binary
  };

  /**
   * @brief HTTP request body container.
   */
  class Body
  {
  public:
    /**
     * @brief Creates an empty body.
     */
    Body() = default;

    /**
     * @brief Creates a body from text data.
     *
     * @param type Body type.
     * @param data Body data.
     * @param contentType Content-Type value.
     */
    Body(
        BodyType type,
        std::string data,
        std::string contentType = {});

    /**
     * @brief Creates a body from binary data.
     *
     * @param data Binary body bytes.
     * @param contentType Content-Type value.
     * @return Body.
     */
    [[nodiscard]] static Body binary(
        std::vector<unsigned char> data,
        std::string contentType = {});

    /**
     * @brief Returns the body type.
     *
     * @return Body type.
     */
    [[nodiscard]] BodyType type() const noexcept;

    /**
     * @brief Returns the body data.
     *
     * The string may contain binary data.
     *
     * @return Body data.
     */
    [[nodiscard]] const std::string &data() const noexcept;

    /**
     * @brief Returns the body as text.
     *
     * @return Body text.
     */
    [[nodiscard]] const std::string &text() const noexcept;

    /**
     * @brief Returns the body bytes.
     *
     * @return Body bytes.
     */
    [[nodiscard]] std::vector<unsigned char> bytes() const;

    /**
     * @brief Returns the content type.
     *
     * @return Content-Type value.
     */
    [[nodiscard]] const std::string &content_type() const noexcept;

    /**
     * @brief Checks whether the body is empty.
     *
     * @return True when there is no body data.
     */
    [[nodiscard]] bool empty() const noexcept;

    /**
     * @brief Returns the body size in bytes.
     *
     * @return Body size.
     */
    [[nodiscard]] std::size_t size() const noexcept;

    /**
     * @brief Checks whether this body has a Content-Type.
     *
     * @return True when Content-Type is not empty.
     */
    [[nodiscard]] bool has_content_type() const noexcept;

  private:
    /**
     * @brief Body type.
     */
    BodyType type_{BodyType::Empty};

    /**
     * @brief Body data.
     */
    std::string data_;

    /**
     * @brief Content-Type value.
     */
    std::string contentType_;
  };

  /**
   * @brief Creates a raw request body.
   *
   * @param data Body data.
   * @param contentType Optional Content-Type value.
   * @return Body.
   */
  [[nodiscard]] Body raw_body(
      std::string_view data,
      std::string contentType = {});

  /**
   * @brief Creates a binary request body.
   *
   * @param data Body bytes.
   * @param contentType Optional Content-Type value.
   * @return Body.
   */
  [[nodiscard]] Body binary_body(
      std::vector<unsigned char> data,
      std::string contentType = {});

  /**
   * @brief Creates a JSON request body.
   *
   * @param json JSON text.
   * @return Body.
   */
  [[nodiscard]] Body json_body(std::string_view json);

  /**
   * @brief Creates a form-url-encoded request body.
   *
   * @param params Form fields.
   * @return Body.
   */
  [[nodiscard]] Body form_body(const Params &params);

  /**
   * @brief Creates a form-url-encoded request body.
   *
   * @param values Form fields.
   * @return Body.
   */
  [[nodiscard]] Body form_body(
      std::initializer_list<std::pair<std::string, std::string>> values);

  /**
   * @brief Converts a BodyType to a stable string name.
   *
   * @param type Body type.
   * @return String name.
   */
  [[nodiscard]] std::string_view to_string(BodyType type) noexcept;

} // namespace vix::requests

#endif // VIX_REQUESTS_BODY_HPP
